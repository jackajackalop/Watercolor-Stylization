#include "GL.hpp"
#include "Load.hpp"

//TextureProgram draws a surface lit by two lights (a distant directional and a hemispherical light) where the surface color is drawn from texture unit 0:
struct MRTBlurVProgram {
	//opengl program object:
	GLuint program = 0;

	//uniform locations:
	MRTBlurVProgram();
};

extern Load< MRTBlurVProgram > mrt_blurV_program;
