#include "GL.hpp"
#include "Load.hpp"

//SurfaceProgram draws to surface_tex so that the r component is the heightmap,
//the g component is the xcomponent of the normal map, the b component is the
//y component of the normal map, and the a component is the tint of the paper
//assuming a light source at (1,1,1)
struct SurfaceProgram {
	//opengl program object:
	GLuint program = 0;

	//uniform locations:
	SurfaceProgram();
};

extern Load< SurfaceProgram > surface_program;
