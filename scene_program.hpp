#include "GL.hpp"
#include "Load.hpp"

//SceneProgram draw the scene lit by the lights specified in GameMode.cpp,
//as well as the control masks, and a depth buffer. The scene lit by lights is
//in color_tex, and it is also changed by hand tremors and some dilution/
//pigment effects
struct SceneProgram {
	//opengl program object:
	GLuint program = 0;

	//uniform locations:
	GLuint object_to_clip_mat4 = -1U;
	GLuint object_to_light_mat4x3 = -1U;
	GLuint normal_to_light_mat3 = -1U;
    GLuint time = -1U;
    GLuint speed = -1U;
    GLuint frequency = -1U;
    GLuint tremor_amount = -1U;
    GLuint clip_units_per_pixel = -1U;
    GLuint viewPos = -1U;
    GLuint dA = -1U;
    GLuint cangiante_variable = -1U;
    GLuint dilution_variable = -1U;

	GLuint sun_direction_vec3 = -1U; //direction *to* sun
	GLuint sun_color_vec3 = -1U;
	GLuint sky_direction_vec3 = -1U; //direction *to* sky
	GLuint sky_color_vec3 = -1U;

	//textures:
	//texture0 - texture for the surface
	//texture1 - texture for spot light shadow map

	SceneProgram();
};

extern Load< SceneProgram > scene_program;
