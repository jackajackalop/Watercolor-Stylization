#include "GL.hpp"
#include "Load.hpp"

//TextureProgram draws a surface lit by two lights (a distant directional and a hemispherical light) where the surface color is drawn from texture unit 0:
struct MRTBlurHProgram {
	//opengl program object:
	GLuint program = 0;

	//uniform locations:
	MRTBlurHProgram();
};

extern Load< MRTBlurHProgram > mrt_blurH_program;
