#include "GL.hpp"
#include "Load.hpp"

//TextureProgram draws a surface lit by two lights (a distant directional and a hemispherical light) where the surface color is drawn from texture unit 0:
struct MRTProgram {
	//opengl program object:
	GLuint program = 0;

	//uniform locations:
	MRTProgram();
};

extern Load< MRTProgram > mrt_program;
