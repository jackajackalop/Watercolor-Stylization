#include "GL.hpp"
#include "Load.hpp"

//TextureProgram draws a surface lit by two lights (a distant directional and a hemispherical light) where the surface color is drawn from texture unit 0:
struct MRTBlurHProgram {
	//opengl program object:
	GLuint program = 0;

	//uniform locations:
    GLuint weights = -1U;
    GLuint blur_amount = -1U;
    GLuint depth_threshold = -1U;
	MRTBlurHProgram();
};

struct MRTBlurVProgram {
	//opengl program object:
	GLuint program = 0;

	//uniform locations:
    GLuint weights = -1U;
    GLuint blur_amount = -1U;
    GLuint depth_threshold = -1U;
	MRTBlurVProgram();
};
extern Load< MRTBlurHProgram > mrt_blurH_program;
extern Load< MRTBlurVProgram > mrt_blurV_program;
