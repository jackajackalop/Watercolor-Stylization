#pragma once

#include "GL.hpp"

#include <string>

//compiles+links an OpenGL shader program from source.
// throws on compilation error.
GLuint compile_program(
	std::string const &vertex_shader_source,
	std::string const &fragment_shader_source);
