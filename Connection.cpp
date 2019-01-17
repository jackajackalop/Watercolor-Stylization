#include "Connection.hpp"

#include <iostream>
#include <cmath>
#include <algorithm>
#include <cassert>
#include <cstring>

//NOTE: much of the sockets code herein is based on http-tweak's single-header http server
// see: https://github.com/ixchow/http-tweak

//Also, some help and examples for getaddrinfo from: https://beej.us/guide/bgnet/html/multi/syscalls.html


//---------------------------------
//Polling helper used by both server and client:
void poll_connections(
	char const *where,
	std::list< Connection > &connections,
	std::function< void(Connection *, Connection::Event event) > const &on_event,
	double timeout,
	SOCKET listen_socket = INVALID_SOCKET) {

	fd_set read_fds, write_fds;
	FD_ZERO(&read_fds);
	FD_ZERO(&write_fds);

	int max = 0;

	//add listen_socket to fd_set if needed:
	if (listen_socket != INVALID_SOCKET) {
		max = std::max(max, int(listen_socket));
		FD_SET(listen_socket, &read_fds);
	}

	//add each connection's socket to read (and possibly write) sets:
	for (auto c : connections) {
		if (c.socket != INVALID_SOCKET) {
			max = std::max(max, int(c.socket));
			FD_SET(c.socket, &read_fds);
			if (!c.send_buffer.empty()) {
				FD_SET(c.socket, &write_fds);
			}
		}
	}

	{ //wait (until timeout) for sockets' data to become available:
		struct timeval tv;
		tv.tv_sec = std::lround(std::floor(timeout));
		tv.tv_usec = std::lround((timeout - std::floor(timeout)) * 1e6);
		//NOTE: on windows nfds is ignored -- https://msdn.microsoft.com/en-us/library/windows/desktop/ms740141(v=vs.85).aspx
		int ret = select(max + 1, &read_fds, &write_fds, NULL, &tv);

		if (ret < 0) {
			std::cerr << "[" << where << "] Select returned an error; will attempt to read/write anyway." << std::endl;
		} else if (ret == 0) {
			//nothing to read or write.
			return;
		}
	}

	//add new connections as needed:
	if (listen_socket != INVALID_SOCKET && FD_ISSET(listen_socket, &read_fds)) {
		SOCKET got = accept(listen_socket, NULL, NULL);
		if (got == INVALID_SOCKET) {
			//oh well.
		} else {
			#ifdef _WIN32
			unsigned long one = 1;
			if (0 == ioctlsocket(got, FIONBIO, &one)) {
			#else
			{
			#endif
				connections.emplace_back();
				connections.back().socket = got;
				std::cerr << "[" << where << "] client connected on " << connections.back().socket << "." << std::endl; //INFO
				if (on_event) on_event(&connections.back(), Connection::OnOpen);
			}
		}
	}

	const uint32_t BufferSize = 20000;
	static thread_local char *buffer = new char[BufferSize];

	//process requests:
	for (auto &c : connections) {
		//only read from valid sockets marked readable:
		if (c.socket == INVALID_SOCKET || !FD_ISSET(c.socket, &read_fds)) continue;

		ssize_t ret = recv(c.socket, buffer, BufferSize, MSG_DONTWAIT);
		if (ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
			//~no problem~ but no data
		} else if (ret <= 0 || ret > (ssize_t)BufferSize) {
			//~problem~ so remove connection
			if (ret == 0) {
				std::cerr << "[" << where << "] port closed, disconnecting." << std::endl;
			} else if (ret < 0) {
				std::cerr << "[" << where << "] recv() returned error " << errno << "(" << strerror(errno) << "), disconnecting." << std::endl;
			} else {
				std::cerr << "[" << where << "] recv() returned strange number of bytes, disconnecting." << std::endl;
			}
			c.close();
			if (on_event) on_event(&c, Connection::OnClose);
		} else { //ret > 0
			c.recv_buffer.insert(c.recv_buffer.end(), buffer, buffer + ret);
			if (on_event) on_event(&c, Connection::OnRecv);
		}
	}

	//process responses:
	for (auto &c : connections) {
		//don't bother with connections unless they are valid, have something to send, and are marked writable:
		if (c.socket == INVALID_SOCKET || c.send_buffer.empty() || !FD_ISSET(c.socket, &write_fds)) continue;
		
		#ifdef _WIN32
		ssize_t ret = send(c.socket, reinterpret_cast< char const * >(c.send_buffer.data()), int(c.send_buffer.size()), MSG_DONTWAIT);
		#else
		ssize_t ret = send(c.socket, reinterpret_cast< char const * >(c.send_buffer.data()), c.send_buffer.size(), MSG_DONTWAIT);
		#endif 
		if (ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
			//~no problem~, but don't keep trying
			break;
		} else if (ret <= 0 || ret > (ssize_t)c.send_buffer.size()) {
			if (ret < 0) {
				std::cerr << "[" << where << "] send() returned error " << errno << ", disconnecting." << std::endl;
			} else { assert(ret == 0 || ret > (ssize_t)c.send_buffer.size());
				std::cerr << "[" << where << "] send() returned strange number of bytes [" << ret << " of " << c.send_buffer.size() << "], disconnecting." << std::endl;
			}
			c.close();
			if (on_event) on_event(&c, Connection::OnClose);
		} else { //ret seems reasonable
			c.send_buffer.erase(c.send_buffer.begin(), c.send_buffer.begin() + ret);
		}
	}

		
}

//---------------------------------


Server::Server(std::string const &port) {

	#ifdef _WIN32
	{ //init winsock:
		WSADATA info;
		if (WSAStartup((2 << 8) | 2, &info) != 0) {
			throw std::runtime_error("WSAStartup failed.");
		}
	}
	#endif

	{ //use getaddrinfo to look up how to bind to port:
		struct addrinfo hints;
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = AI_PASSIVE;

		struct addrinfo *res = nullptr;
		int ret = getaddrinfo(NULL, port.c_str(), &hints, &res);
		if (ret != 0) {
			throw std::runtime_error("getaddrinfo error: " + std::string(gai_strerror(ret)));
		}

		std::cout << "[Server::Server] binding to " << port << ":" << std::endl;
		//based on example code in the 'man getaddrinfo' man page on OSX:
		for (struct addrinfo *info = res; info != nullptr; info = info->ai_next) {
			{ //DEBUG: dump info about this address:
				std::cout << "\ttrying ";
				char ip[INET6_ADDRSTRLEN];
				if (info->ai_family == AF_INET) {
					struct sockaddr_in *s = reinterpret_cast< struct sockaddr_in * >(info->ai_addr);
					inet_ntop(res->ai_family, &s->sin_addr, ip, sizeof(ip));
					std::cout << ip << ":" << ntohs(s->sin_port);
				} else if (info->ai_family == AF_INET6) {
					struct sockaddr_in6 *s = reinterpret_cast< struct sockaddr_in6 * >(info->ai_addr);
					inet_ntop(res->ai_family, &s->sin6_addr, ip, sizeof(ip));
					std::cout << ip << ":" << ntohs(s->sin6_port);
				} else {
					std::cout << "[unknown ai_family]";
				}
				std::cout << "... "; std::cout.flush();
			}

			SOCKET s = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
			if (s == INVALID_SOCKET) {
				std::cout << "(failed to create socket: " << strerror(errno) << ")" << std::endl;
				continue;
			}

			{ //make it okay to reuse port:
				#ifdef _WIN32
				BOOL one = TRUE;
				int ret = setsockopt(s, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast< const char * >(&one), sizeof(one));
				#else
				int one = 1;
				int ret = setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
				#endif
				if (ret != 0) {
					std::cout << "[note: couldn't set SO_REUSEADDR] " << std::endl;
				}
			}

			int ret = bind(s, info->ai_addr, int(info->ai_addrlen));
			if (ret < 0) {
				std::cout << "(failed to bind: " << strerror(errno) << ")" << std::endl;
				continue;
			}
			std::cout << "success!" << std::endl;

			listen_socket = s;
			break;
		}

		freeaddrinfo(res);
	}

	if (listen_socket == INVALID_SOCKET) {
		throw std::runtime_error("Failed to bind to port " + port);
	}

	{ //listen on socket
		int ret = ::listen(listen_socket, 5);
		if (ret < 0) {
			closesocket(listen_socket);
			throw std::system_error(errno, std::system_category(), "failed to listen on socket");
		}
	}
}

void Server::poll(std::function< void(Connection *, Connection::Event event) > const &on_event, double timeout) {
	poll_connections("Server::poll", connections, on_event, timeout, listen_socket);

	//reap closed clients:
	for (auto connection = connections.begin(); connection != connections.end(); /*later*/) {
		auto old = connection;
		++connection;
		if (old->socket == INVALID_SOCKET) {
			connections.erase(old);
		}
	}
}

Client::Client(std::string const &host, std::string const &port) : connections(1), connection(connections.front()) {
	#ifdef _WIN32
	{ //init winsock:
		WSADATA info;
		if (WSAStartup((2 << 8) | 2, &info) != 0) {
			throw std::runtime_error("WSAStartup failed.");
		}
	}
	#endif

	{ //use getaddrinfo to look up how to bind to host/port:
		struct addrinfo hints;
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;

		struct addrinfo *res = nullptr;
		int ret = getaddrinfo(host.c_str(), port.c_str(), &hints, &res);
		if (ret != 0) {
			throw std::runtime_error("getaddrinfo error: " + std::string(gai_strerror(ret)));
		}

		std::cout << "[Client::Client] connecting to " << host << ":" << port << ":" << std::endl;
		//based on example code in the 'man getaddrinfo' man page on OSX:
		for (struct addrinfo *info = res; info != nullptr; info = info->ai_next) {
			{ //DEBUG: dump info about this address:
				std::cout << "\ttrying ";
				char ip[INET6_ADDRSTRLEN];
				if (info->ai_family == AF_INET) {
					struct sockaddr_in *s = reinterpret_cast< struct sockaddr_in * >(info->ai_addr);
					inet_ntop(res->ai_family, &s->sin_addr, ip, sizeof(ip));
					std::cout << ip << ":" << ntohs(s->sin_port);
				} else if (info->ai_family == AF_INET6) {
					struct sockaddr_in6 *s = reinterpret_cast< struct sockaddr_in6 * >(info->ai_addr);
					inet_ntop(res->ai_family, &s->sin6_addr, ip, sizeof(ip));
					std::cout << ip << ":" << ntohs(s->sin6_port);
				} else {
					std::cout << "[unknown ai_family]";
				}
				std::cout << "... "; std::cout.flush();
			}

			SOCKET s = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
			if (s == INVALID_SOCKET) {
				std::cout << "(failed to create socket: " << strerror(errno) << ")" << std::endl;
				continue;
			}
			int ret = connect(s, info->ai_addr, int(info->ai_addrlen));
			if (ret < 0) {
				std::cout << "(failed to connect: " << strerror(errno) << ")" << std::endl;
				continue;
			}
			std::cout << "success!" << std::endl;

			connection.socket = s;
			break;
		}

		freeaddrinfo(res);

		if (!connection) {
			throw std::runtime_error("Failed to connect to any of the addresses tried for server.");
		}
	}
}


void Client::poll(std::function< void(Connection *, Connection::Event event) > const &on_event, double timeout) {
	poll_connections("Client::poll", connections, on_event, timeout, INVALID_SOCKET);
}

