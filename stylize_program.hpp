#include "GL.hpp"
#include "Load.hpp"

//StylizeProgram draws a surface lit by two lights (a distant directional and a hemispherical light) where the surface color is drawn from texture unit 0:
struct StylizeProgram {
	//opengl program object:
	GLuint program = 0;
    GLuint density_amount = -1U;

	//uniform locations:
	StylizeProgram();
};

extern Load< StylizeProgram > stylize_program;
