#include "GL.hpp"
#include "Load.hpp"

//TextureProgram draws a surface lit by two lights (a distant directional and a hemispherical light) where the surface color is drawn from texture unit 0:
struct TextureProgram {
	//opengl program object:
	GLuint program = 0;

	//uniform locations:
	GLuint object_to_clip_mat4 = -1U;
	GLuint object_to_light_mat4x3 = -1U;
	GLuint normal_to_light_mat3 = -1U;

	GLuint sun_direction_vec3 = -1U; //direction *to* sun
	GLuint sun_color_vec3 = -1U;
	GLuint sky_direction_vec3 = -1U; //direction *to* sky
	GLuint sky_color_vec3 = -1U;

	GLuint spot_position_vec3 = -1U;
	GLuint spot_direction_vec3 = -1U; //direction *from* spotlight
	GLuint spot_color_vec3 = -1U;
	GLuint spot_outer_inner_vec2 = -1U; //color fades from zero to one as dot(spot_direction, spot_to_position) varies from outer_inner.x to outer_inner.y
	GLuint light_to_spot_mat4 = -1U; //projects from lighting space (/world space) to spot light depth map space

	//textures:
	//texture0 - texture for the surface
	//texture1 - texture for spot light shadow map

	TextureProgram();
};

extern Load< TextureProgram > texture_program;
