#include "GL.hpp"
#include "Load.hpp"

//MRTBlur*Program does a horizontal pass and a vertical pass of gaussian blur
//and bilateral blur using some macro trickery
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
