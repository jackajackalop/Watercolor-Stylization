#include "texture_program.hpp"

#include "compile_program.hpp"
#include "gl_errors.hpp"

TextureProgram::TextureProgram() {
	program = compile_program(
		"#version 330\n"
		"uniform mat4 object_to_clip;\n"
		"uniform mat4x3 object_to_light;\n"
		"uniform mat3 normal_to_light;\n"
		"uniform mat4 light_to_spot;\n"
		"layout(location=0) in vec4 Position;\n" //note: layout keyword used to make sure that the location-0 attribute is always bound to something
		"in vec3 Normal;\n"
		"in vec4 Color;\n"
		"in vec2 TexCoord;\n"
		"out vec3 position;\n"
		"out vec3 normal;\n"
		"out vec4 color;\n"
		"out vec2 texCoord;\n"
		"out vec4 spotPosition;\n"
		"void main() {\n"
		"	gl_Position = object_to_clip * Position;\n"
		"	position = object_to_light * Position;\n"
		"	spotPosition = light_to_spot * vec4(position, 1.0);\n"
		"	normal = normal_to_light * Normal;\n"
		"	color = Color;\n"
		"	texCoord = TexCoord;\n"
		"}\n"
		,
		"#version 330\n"
		"uniform vec3 sun_direction;\n"
		"uniform vec3 sun_color;\n"
		"uniform vec3 sky_direction;\n"
		"uniform vec3 sky_color;\n"
		"uniform vec3 spot_position;\n"
		"uniform vec3 spot_direction;\n"
		"uniform vec3 spot_color;\n"
		"uniform vec2 spot_outer_inner;\n"
		"uniform sampler2D tex;\n"
		"uniform sampler2DShadow spot_depth_tex;\n"
		"in vec3 position;\n"
		"in vec3 normal;\n"
		"in vec4 color;\n"
		"in vec2 texCoord;\n"
		"in vec4 spotPosition;\n"
		"out vec4 fragColor;\n"
		"void main() {\n"
		"	vec3 total_light = vec3(0.0, 0.0, 0.0);\n"
		"	vec3 n = normalize(normal);\n"
		"	{ //sky (hemisphere) light:\n"
		"		vec3 l = sky_direction;\n"
		"		float nl = 0.5 + 0.5 * dot(n,l);\n"
		"		total_light += nl * sky_color;\n"
		"	}\n"
		"	{ //sun (directional) light:\n"
		"		vec3 l = sun_direction;\n"
		"		float nl = max(0.0, dot(n,l));\n"
		"		total_light += nl * sun_color;\n"
		"	}\n"
		"	{ //spot (point with fov + shadow map) light:\n"
		"		vec3 l = normalize(spot_position - position);\n"
		"		float nl = max(0.0, dot(n,l));\n"
		"		//TODO: look up shadow map\n"
		"		float d = dot(l,-spot_direction);\n"
		"		float amt = smoothstep(spot_outer_inner.x, spot_outer_inner.y, d);\n"
		"		float shadow = textureProj(spot_depth_tex, spotPosition);\n"
		"		total_light += shadow * nl * amt * spot_color;\n"
		//"		fragColor = vec4(s,s,s, 1.0);\n" //DEBUG: just show shadow
		"	}\n"

		"	fragColor = texture(tex, texCoord) * vec4(color.rgb * total_light, color.a);\n"
		"}\n"
	);

	object_to_clip_mat4 = glGetUniformLocation(program, "object_to_clip");
	object_to_light_mat4x3 = glGetUniformLocation(program, "object_to_light");
	normal_to_light_mat3 = glGetUniformLocation(program, "normal_to_light");

	sun_direction_vec3 = glGetUniformLocation(program, "sun_direction");
	sun_color_vec3 = glGetUniformLocation(program, "sun_color");
	sky_direction_vec3 = glGetUniformLocation(program, "sky_direction");
	sky_color_vec3 = glGetUniformLocation(program, "sky_color");

	spot_position_vec3 = glGetUniformLocation(program, "spot_position");
	spot_direction_vec3 = glGetUniformLocation(program, "spot_direction");
	spot_color_vec3 = glGetUniformLocation(program, "spot_color");
	spot_outer_inner_vec2 = glGetUniformLocation(program, "spot_outer_inner");

	light_to_spot_mat4 = glGetUniformLocation(program, "light_to_spot");

	glUseProgram(program);

	GLuint tex_sampler2D = glGetUniformLocation(program, "tex");
	glUniform1i(tex_sampler2D, 0);

	GLuint spot_depth_tex_sampler2D = glGetUniformLocation(program, "spot_depth_tex");
	glUniform1i(spot_depth_tex_sampler2D, 1);

	glUseProgram(0);

	GL_ERRORS();
}

Load< TextureProgram > texture_program(LoadTagInit, [](){
	return new TextureProgram();
});
