#pragma once

#include <vector>
#include <unordered_map>
#include <map>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp> //allows the use of 'uvec2' as an unordered_map key

struct WalkMesh {
	//Walk mesh will keep track of triangles, vertices:
	std::vector< glm::vec3 > vertices;
	std::vector< glm::vec3 > normals; //normals for interpolated 'up' direction
	std::vector< glm::uvec3 > triangles; //CCW-oriented

	//This "next vertex" map includes [a,b]->c, [b,c]->a, and [c,a]->b for each triangle, and is useful for checking what's over an edge from a given point:
	std::unordered_map< glm::uvec2, uint32_t > next_vertex;


	//Construct new WalkMesh and build next_vertex structure:
	WalkMesh(std::vector< glm::vec3 > const &vertices_, std::vector< glm::vec3 > const &normals_, std::vector< glm::uvec3 > const &triangles_);

	struct WalkPoint {
		glm::uvec3 triangle = glm::uvec3(-1U); //indices of current triangle
		glm::vec3 weights = glm::vec3(std::numeric_limits< float >::quiet_NaN()); //barycentric coordinates for current point
	};

	//used to initialize walking -- finds the closest point on the walk mesh:
	// (should only need to call this at the start of a level)
	WalkPoint start(glm::vec3 const &world_point) const;

	//used to update walk point:
	void walk(WalkPoint &wp, glm::vec3 const &step) const;

	//used to read back results of walking:
	glm::vec3 world_point(WalkPoint const &wp) const {
		return wp.weights.x * vertices[wp.triangle.x]
		     + wp.weights.y * vertices[wp.triangle.y]
		     + wp.weights.z * vertices[wp.triangle.z];
	}

	glm::vec3 world_normal(WalkPoint const &wp) const {
		return glm::normalize(
			wp.weights.x * normals[wp.triangle.x]
		     + wp.weights.y * normals[wp.triangle.y]
		     + wp.weights.z * normals[wp.triangle.z]
		);
	}

};

struct WalkMeshes {
	//load a list of named WalkMeshes from a file:
	WalkMeshes(std::string const &filename);

	//retrieve a WalkMesh by name:
	WalkMesh const &lookup(std::string const &name) const;

	//internals:
	std::map< std::string, WalkMesh > meshes;
};

/*
// The intent is that game code will work something like this:

Load< WalkMesh > walk_mesh;

Game {
	WalkPoint walk_point;
}
Game::Game() {
	//...
	walk_point = walk_mesh->start(level_start_position);
}

Game::update(float elapsed) {
	//update position on walk mesh:
	glm::vec3 step = player_forward * speed * elapsed;
	walk_mesh->walk(walk_point, step);

	//update player position:
	player_at = walk_mesh->world_point(walk_point);

	//update player orientation:
	glm::vec3 old_player_up = player_up;
	player_up = walk_mesh->world_normal(walk_point);

	glm::quat orientation_change = (compute rotation that takes old_player_up to player_up)
	player_forward = orientation_change * player_forward;

	//make sure player_forward is perpendicular to player_up (the earlier rotation should ensure that, but it might drift over time):
	player_forward = glm::normalize(player_forward - player_up * glm::dot(player_up, player_forward));

	//compute rightward direction from forward and up:
	player_right = glm::cross(player_forward, player_up);

}
*/
