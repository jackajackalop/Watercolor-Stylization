#include "GL.hpp"
#include "Load.hpp"

//SurfaceProgram draws a surface lit by two lights (a distant directional and a hemispherical light) where the surface color is drawn from texture unit 0:
struct SurfaceProgram {
	//opengl program object:
	GLuint program = 0;

	//uniform locations:
	SurfaceProgram();
};

extern Load< SurfaceProgram > surface_program;
