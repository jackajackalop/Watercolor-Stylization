#include "GL.hpp"
#include "Load.hpp"

//TextureProgram draws a surface lit by two lights (a distant directional and a hemispherical light) where the surface color is drawn from texture unit 0:
struct MRTBlurProgram {
	//opengl program object:
	GLuint program = 0;

	//uniform locations:
	MRTBlurProgram();
};

extern Load< MRTBlurProgram > mrt_blur_program;
