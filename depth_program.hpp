#include "GL.hpp"
#include "Load.hpp"

struct DepthProgram {
	//opengl program object:
	GLuint program = 0;

	//uniform locations:
	GLuint object_to_clip_mat4 = -1U;

	DepthProgram();
};

extern Load< DepthProgram > depth_program;
