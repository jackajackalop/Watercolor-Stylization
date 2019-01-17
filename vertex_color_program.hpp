#include "GL.hpp"
#include "Load.hpp"

struct VertexColorProgram {
	//opengl program object:
	GLuint program = 0;

	//uniform locations:
	GLuint object_to_clip_mat4 = -1U;
	GLuint object_to_light_mat4x3 = -1U;
	GLuint normal_to_light_mat3 = -1U;
	GLuint sun_direction_vec3 = -1U;
	GLuint sun_color_vec3 = -1U;
	GLuint sky_direction_vec3 = -1U;
	GLuint sky_color_vec3 = -1U;

	VertexColorProgram();
};

extern Load< VertexColorProgram > vertex_color_program;
