#include "GL.hpp"
#include "Load.hpp"

//StylizeProgram combines the effects of paper distortion, paper granulation,
//edge darkening, and color bleeding into final_tex using color_tex,
//control_tex, blurred_tex, bleeded_tex, and surface_tex
struct StylizeProgram {
	//opengl program object:
	GLuint program = 0;
    GLuint density_amount = -1U;

	//uniform locations:
	StylizeProgram();
};

extern Load< StylizeProgram > stylize_program;
