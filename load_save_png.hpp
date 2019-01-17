#pragma once

#include <glm/glm.hpp>

#include <string>
#include <vector>
#include <stdint.h>

/*
 * Load and save PNG files.
 */

enum OriginLocation {
	LowerLeftOrigin,
	UpperLeftOrigin,
};

//NOTE: load_png will throw on error
void load_png(std::string filename, glm::uvec2 *size, std::vector< glm::u8vec4 > *data, OriginLocation origin);
void save_png(std::string filename, glm::uvec2 size, glm::u8vec4 const *data, OriginLocation origin);
