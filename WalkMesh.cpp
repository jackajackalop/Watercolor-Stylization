#include "WalkMesh.hpp"

#include "read_chunk.hpp"

#include <glm/gtx/norm.hpp>

#include <iostream>
#include <fstream>
#include <algorithm>
#include <string>

WalkMesh::WalkMesh(std::vector< glm::vec3 > const &vertices_, std::vector< glm::vec3 > const &normals_, std::vector< glm::uvec3 > const &triangles_)
	: vertices(vertices_), normals(normals_), triangles(triangles_) {

	//construct next_vertex map (maps each edge to the next vertex in the triangle):
	next_vertex.reserve(triangles.size()*3);
	auto do_next = [this](uint32_t a, uint32_t b, uint32_t c) {
		auto ret = next_vertex.insert(std::make_pair(glm::uvec2(a,b), c));
		assert(ret.second);
	};
	for (auto const &tri : triangles) {
		do_next(tri.x, tri.y, tri.z);
		do_next(tri.y, tri.z, tri.x);
		do_next(tri.z, tri.x, tri.y);
	}

	//DEBUG: are vertex normals consistent with geometric normals?
	for (auto const &tri : triangles) {
		glm::vec3 const &a = vertices[tri.x];
		glm::vec3 const &b = vertices[tri.y];
		glm::vec3 const &c = vertices[tri.z];
		glm::vec3 out = glm::normalize(glm::cross(b-a, c-a));

		float da = glm::dot(out, normals[tri.x]);
		float db = glm::dot(out, normals[tri.y]);
		float dc = glm::dot(out, normals[tri.z]);

		assert(da > 0.1f && db > 0.1f && dc > 0.1f);
	}
}

WalkMesh::WalkPoint WalkMesh::start(glm::vec3 const &world_point) const {
	WalkPoint closest;
	float closest_dis2 = std::numeric_limits< float >::infinity();
	for (auto const &tri : triangles) {
		glm::vec3 const &a = vertices[tri.x];
		glm::vec3 const &b = vertices[tri.y];
		glm::vec3 const &c = vertices[tri.z];

		//figure out barycentric coordinates for point:
		//project to plane of triangle:
		glm::vec3 out = glm::cross(b-a, c-a);
		glm::vec3 pt = world_point - out * (glm::dot(out, world_point) / glm::dot(out, out));

		//figure out barycentric coordinates using signed triangle areas:
		glm::vec3 coords = glm::vec3(
			glm::dot(out, glm::cross(c-b, pt-b)),
			glm::dot(out, glm::cross(a-c, pt-c)),
			glm::dot(out, glm::cross(b-a, pt-a))
		) / glm::dot(out, glm::cross(b-a, c-a));

		//is point inside triangle?
		if (coords.x >= 0.0f && coords.y >= 0.0f && coords.z >= 0.0f) {
			//yes, point is inside triangle.
			float dis2 = glm::length2(world_point - pt);
			if (dis2 < closest_dis2) {
				closest_dis2 = dis2;
				closest.triangle = tri;
				closest.weights = coords;
			}
		} else {
			//check triangle vertices and edges:
			auto check_edge = [&world_point, &closest, &closest_dis2, this](uint32_t ai, uint32_t bi, uint32_t ci) {
				glm::vec3 const &a = vertices[ai];
				glm::vec3 const &b = vertices[bi];

				//find closest point on line segment ab:
				float along = glm::dot(world_point-a, b-a);
				float max = glm::dot(b-a, b-a);
				glm::vec3 pt;
				glm::vec3 coords;
				if (along < 0.0f) {
					pt = a;
					coords = glm::vec3(1.0f, 0.0f, 0.0f);
				} else if (along > max) {
					pt = b;
					coords = glm::vec3(0.0f, 1.0f, 0.0f);
				} else {
					float amt = along / max;
					pt = glm::mix(a, b, amt);
					coords = glm::vec3(1.0f - amt, amt, 0.0f);
				}

				float dis2 = glm::length2(world_point - pt);
				if (dis2 < closest_dis2) {
					closest_dis2 = dis2;
					closest.triangle = glm::uvec3(ai, bi, ci);
					closest.weights = coords;
				}
			};
			check_edge(tri.x, tri.y, tri.z);
			check_edge(tri.y, tri.z, tri.x);
			check_edge(tri.z, tri.x, tri.y);
		}
	}
	return closest;
}

void WalkMesh::walk(WalkMesh::WalkPoint &wp, glm::vec3 const &step) const {

	glm::vec3 remain = step;

	uint32_t iter = 0;
	while (remain != glm::vec3(0.0f)) {
		if (iter > 10) {
			std::cerr << "WARNING: Couldn't resolve step in ten iterations, discarding the rest." << std::endl;
			break;
		}
		iter += 1;

		glm::vec3 remain_coords;
		{ //project 'remain' to current triangle:
			glm::vec3 const &a = vertices[wp.triangle.x];
			glm::vec3 const &b = vertices[wp.triangle.y];
			glm::vec3 const &c = vertices[wp.triangle.z];

			//project to plane of triangle:
			glm::vec3 out = glm::cross(b-a, c-a);
			glm::vec3 pt = remain - out * (glm::dot(out, remain) / glm::dot(out, out));

			assert(pt == pt); //pt isn't NaN, right?

			//figure out barycentric coordinates for remain:
			glm::vec3 coords = glm::vec3(
				glm::dot(out, glm::cross(c-b, pt)),
				glm::dot(out, glm::cross(a-c, pt)),
				glm::dot(out, glm::cross(b-a, pt))
			) / glm::dot(out, glm::cross(b-a, c-a));

			assert(coords.x == coords.x && coords.y == coords.y && coords.z == coords.z); //riiiight?

			remain_coords = coords;

			assert(remain_coords.x == remain_coords.x); //remain_coords shouldn't be NaN
		}

		float t = 1.0f;
		glm::uvec2 edge = glm::uvec2(-1U); uint32_t other = -1U;
		glm::vec2 edge_coords = glm::vec2(std::numeric_limits< float >::quiet_NaN());
		{ //figure out when (if ever) and where an edge is crossed:
			#define TEST_COORD( C, A, B ) \
				if (remain_coords.C < 0.0f) { \
					float test = std::max(0.0f, -wp.weights.C / remain_coords.C); \
					if (test < t) { \
						t = test; \
						edge = glm::uvec2(wp.triangle.A, wp.triangle.B); other = wp.triangle.C; \
						edge_coords = glm::vec2(t * remain_coords.A + wp.weights.A, t * remain_coords.B + wp.weights.B); \
					} \
				}
			TEST_COORD( x, y, z );
			TEST_COORD( y, z, x );
			TEST_COORD( z, x, y );
			#undef TEST_COORD
		}
		assert(t == t); //makes sure t isn't NaN

		//didn't hit an edge -- move and done.
		if (t >= 1.0f) {
			wp.weights += remain_coords;
			remain_coords = glm::vec3(0.0f);
			break;
		}
		assert(t >= 0.0f);
		assert(edge_coords == edge_coords); //edge_coords isn't NaN

		//consume the first 't' of step:
		wp.weights += remain_coords * t;
		remain *= (1.0f - t);

		//is edge solid?
		auto f = next_vertex.find(glm::uvec2(edge.y, edge.x));
		if (f == next_vertex.end()) {
			//if yes, move remain to point (slightly) inward:
			glm::vec3 along = glm::normalize(vertices[edge.y] - vertices[edge.x]);
			glm::vec3 in = vertices[other] - vertices[edge.x];
			in = glm::normalize(in - along * glm::dot(along, in));

			float d = glm::dot(in, remain);

			remain = remain + in * (0.01f - d);

			//NOTE: this probably results in an infinite loop when walking into a corner.
		} else {
			//if no, move to new triangle:
			assert(f->second != other);

			//update triangle and weights:
			wp.triangle = glm::uvec3(edge.y, edge.x, f->second);
			wp.weights = glm::vec3(edge_coords.y, edge_coords.x, 0.0f);

			//rotate 'remain' around edge:
			glm::vec3 along = glm::normalize(vertices[edge.y] - vertices[edge.x]);
			glm::vec3 to_old_other = vertices[other] - vertices[edge.x];
			to_old_other = glm::normalize(to_old_other - along * glm::dot(along, to_old_other));

			glm::vec3 to_new_other = vertices[f->second] - vertices[edge.y];
			to_new_other = glm::normalize(to_new_other - along * glm::dot(along, to_new_other));

			float d = glm::dot(remain, -to_old_other); //amount of 'remain' sticking out of old triangle

			remain -= d * -to_old_other; //remove 'remain' sticking out in plane of old triangle
			remain += d * to_new_other; //add it back in sticking out in plane of new triangle
		}
	}
}


WalkMeshes::WalkMeshes(std::string const &filename) {
	std::ifstream file(filename, std::ios::binary);

	std::vector< glm::vec3 > vertices;
	read_chunk(file, "p...", &vertices);

	std::vector< glm::vec3 > normals;
	read_chunk(file, "n...", &normals);

	std::vector< glm::uvec3 > triangles;
	read_chunk(file, "tri0", &triangles);

	std::vector< char > names;
	read_chunk(file, "str0", &names);

	struct IndexEntry {
		uint32_t name_begin, name_end;
		uint32_t vertex_begin, vertex_end;
		uint32_t triangle_begin, triangle_end;
	};

	std::vector< IndexEntry > index;
	read_chunk(file, "idxA", &index);

	if (file.peek() != EOF) {
		std::cerr << "WARNING: trailing data in walkmesh file '" << filename << "'" << std::endl;
	}

	//-----------------

	if (vertices.size() != normals.size()) {
		throw std::runtime_error("Mis-matched position and normal sizes in '" + filename + "'");
	}

	for (auto const &e : index) {
		if (!(e.name_begin <= e.name_end && e.name_end <= names.size())) {
			throw std::runtime_error("Invalid name indices in index of '" + filename + "'");
		}
		if (!(e.vertex_begin <= e.vertex_end && e.vertex_end <= vertices.size())) {
			throw std::runtime_error("Invalid vertex indices in index of '" + filename + "'");
		}
		if (!(e.triangle_begin <= e.triangle_end && e.triangle_end <= triangles.size())) {
			throw std::runtime_error("Invalid triangle indices in index of '" + filename + "'");
		}

		//copy vertices/normals:
		std::vector< glm::vec3 > wm_vertices(vertices.begin() + e.vertex_begin, vertices.begin() + e.vertex_end);
		std::vector< glm::vec3 > wm_normals(normals.begin() + e.vertex_begin, normals.begin() + e.vertex_end);

		//remap triangles:
		std::vector< glm::uvec3 > wm_triangles; wm_triangles.reserve(e.triangle_end - e.triangle_begin);
		for (uint32_t ti = e.triangle_begin; ti != e.triangle_end; ++ti) {
			if (!( (e.vertex_begin <= triangles[ti].x && triangles[ti].x < e.vertex_end)
			    && (e.vertex_begin <= triangles[ti].y && triangles[ti].y < e.vertex_end)
			    && (e.vertex_begin <= triangles[ti].z && triangles[ti].z < e.vertex_end) )) {
				throw std::runtime_error("Invalid triangle in '" + filename + "'");
			}
			wm_triangles.emplace_back(
				triangles[ti].x - e.vertex_begin,
				triangles[ti].y - e.vertex_begin,
				triangles[ti].z - e.vertex_begin
			);
		}
		
		std::string name(names.begin() + e.name_begin, names.begin() + e.name_end);

		auto ret = meshes.emplace(name, WalkMesh(wm_vertices, wm_normals, wm_triangles));
		if (!ret.second) {
			throw std::runtime_error("WalkMesh with duplicated name '" + name + "' in '" + filename + "'");
		}

	}
}

WalkMesh const &WalkMeshes::lookup(std::string const &name) const {
	auto f = meshes.find(name);
	if (f == meshes.end()) {
		throw std::runtime_error("WalkMesh with duplicated name '" + name + "' not found.");
	}
	return f->second;
}
