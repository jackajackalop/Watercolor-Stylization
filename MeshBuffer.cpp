#include "MeshBuffer.hpp"
#include "read_chunk.hpp"

#include <glm/glm.hpp>

#include <stdexcept>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <set>
#include <cstddef>

MeshBuffer::MeshBuffer(std::string const &filename) {
	glGenBuffers(1, &vbo);

	std::ifstream file(filename, std::ios::binary);

	GLuint total = 0;
	//read + upload data chunk:
	if (filename.size() >= 2 && filename.substr(filename.size()-2) == ".p") {
		struct Vertex {
			glm::vec3 Position;
		};
		static_assert(sizeof(Vertex) == 3*4, "Vertex is packed.");

		std::vector< Vertex > data;
		read_chunk(file, "p...", &data);

		//upload data:
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(Vertex), data.data(), GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		total = GLuint(data.size()); //store total for later checks on index

		//store attrib locations:
		Position = Attrib(3, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, Position));

	} else if (filename.size() >= 3 && filename.substr(filename.size()-3) == ".pn") {
		struct Vertex {
			glm::vec3 Position;
			glm::vec3 Normal;
		};
		static_assert(sizeof(Vertex) == 3*4+3*4, "Vertex is packed.");

		std::vector< Vertex > data;
		read_chunk(file, "pn..", &data);

		//upload data:
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(Vertex), data.data(), GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		total = GLuint(data.size()); //store total for later checks on index

		//store attrib locations:
		Position = Attrib(3, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, Position));
		Normal = Attrib(3, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, Normal));

	} else if (filename.size() >= 4 && filename.substr(filename.size()-4) == ".pnc") {
		struct Vertex {
			glm::vec3 Position;
			glm::vec3 Normal;
			glm::u8vec4 Color;
		};
		static_assert(sizeof(Vertex) == 3*4+3*4+4*1, "Vertex is packed.");

		std::vector< Vertex > data;
		read_chunk(file, "pnc.", &data);

		//upload data:
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(Vertex), data.data(), GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		total = GLuint(data.size()); //store total for later checks on index

		//store attrib locations:
		Position = Attrib(3, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, Position));
		Normal = Attrib(3, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, Normal));
		Color = Attrib(4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), offsetof(Vertex, Color));

	} else if (filename.size() >= 5 && filename.substr(filename.size()-5) == ".pnct") {
		struct Vertex {
			glm::vec3 Position;
			glm::vec3 Normal;
			glm::u8vec4 Color;
			glm::vec2 TexCoord;
		};
		static_assert(sizeof(Vertex) == 3*4+3*4+4*1+2*4, "Vertex is packed.");

		std::vector< Vertex > data;
		read_chunk(file, "pnct", &data);

		//upload data:
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(Vertex), data.data(), GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		total = GLuint(data.size()); //store total for later checks on index

		//store attrib locations:
		Position = Attrib(3, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, Position));
		Normal = Attrib(3, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, Normal));
		Color = Attrib(4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), offsetof(Vertex, Color));
		TexCoord = Attrib(2, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, TexCoord));

	} else {
		throw std::runtime_error("Unknown file type '" + filename + "'");
	}

	std::vector< char > strings;
	read_chunk(file, "str0", &strings);

	{ //read index chunk, add to meshes:
		struct IndexEntry {
			uint32_t name_begin, name_end;
			uint32_t vertex_begin, vertex_end;
		};
		static_assert(sizeof(IndexEntry) == 16, "Index entry should be packed");

		std::vector< IndexEntry > index;
		read_chunk(file, "idx0", &index);

		for (auto const &entry : index) {
			if (!(entry.name_begin <= entry.name_end && entry.name_end <= strings.size())) {
				throw std::runtime_error("index entry has out-of-range name begin/end");
			}
			if (!(entry.vertex_begin <= entry.vertex_end && entry.vertex_end <= total)) {
				throw std::runtime_error("index entry has out-of-range vertex start/count");
			}
			std::string name(&strings[0] + entry.name_begin, &strings[0] + entry.name_end);
			Mesh mesh;
			mesh.start = entry.vertex_begin;
			mesh.count = entry.vertex_end - entry.vertex_begin;
			bool inserted = meshes.insert(std::make_pair(name, mesh)).second;
			if (!inserted) {
				std::cerr << "WARNING: mesh name '" + name + "' in filename '" + filename + "' collides with existing mesh." << std::endl;
			}
		}
	}

	if (file.peek() != EOF) {
		std::cerr << "WARNING: trailing data in mesh file '" << filename << "'" << std::endl;
	}

	/* //DEBUG:
	std::cout << "File '" << filename << "' contained meshes";
	for (auto const &m : meshes) {
		if (&m.second == &meshes.rbegin()->second && meshes.size() > 1) std::cout << " and";
		std::cout << " '" << m.first << "'";
		if (&m.second != &meshes.rbegin()->second) std::cout << ",";
	}
	std::cout << std::endl;
	*/
}

const MeshBuffer::Mesh &MeshBuffer::lookup(std::string const &name) const {
	auto f = meshes.find(name);
	if (f == meshes.end()) {
		throw std::runtime_error("Looking up mesh '" + name + "' that doesn't exist.");
	}
	return f->second;
}

GLuint MeshBuffer::make_vao_for_program(GLuint program) const {
	//create a new vertex array object:
	GLuint vao = 0;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	//Try to bind all attributes in this buffer:
	std::set< GLuint > bound;
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	auto bind_attribute = [&](char const *name, MeshBuffer::Attrib const &attrib) {
		if (attrib.size == 0) return; //don't bind empty attribs
		GLint location = glGetAttribLocation(program, name);
		if (location == -1) {
			std::cerr << "WARNING: attribute '" << name << "' in mesh buffer isn't active in program." << std::endl;
		} else {
			glVertexAttribPointer(location, attrib.size, attrib.type, attrib.normalized, attrib.stride, (GLbyte *)0 + attrib.offset);
			glEnableVertexAttribArray(location);
			bound.insert(location);
		}
	};
	bind_attribute("Position", Position);
	bind_attribute("Normal", Normal);
	bind_attribute("Color", Color);
	bind_attribute("TexCoord", TexCoord);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	//Check that all active attributes were bound:
	GLint active = 0;
	glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &active);
	assert(active >= 0 && "Doesn't makes sense to have negative active attributes.");
	for (GLuint i = 0; i < GLuint(active); ++i) {
		GLchar name[100];
		GLint size = 0;
		GLenum type = 0;
		glGetActiveAttrib(program, i, 100, NULL, &size, &type, name);
		name[99] = '\0';
		GLint location = glGetAttribLocation(program, name);
		if (!bound.count(GLuint(location))) {
			throw std::runtime_error("ERROR: active attribute '" + std::string(name) + "' in program is not bound.");
		}
	}

	return vao;
}
