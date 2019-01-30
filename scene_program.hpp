#include "GL.hpp"
#include "Load.hpp"

//SceneProgram draws a surface lit by two lights (a distant directional and a hemispherical light) where the surface color is drawn from texture unit 0:
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
    GLuint viewdir = -1U;

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
