#include "scene_program.hpp"

#include "compile_program.hpp"
#include "gl_errors.hpp"

SceneProgram::SceneProgram() {
	program = compile_program(
		"#version 330\n"
		"uniform mat4 object_to_clip;\n"
		"uniform mat4x3 object_to_light;\n"
		"uniform mat3 normal_to_light;\n"
        "uniform float time;\n"
        "uniform float speed;\n"
        "uniform float frequency;\n"
        "uniform float tremor_amount;\n"
        "uniform vec2 clip_units_per_pixel;\n"
        "uniform vec3 viewPos;\n"
		"layout(location=0) in vec4 Position;\n"
        //note: layout keyword used to make sure that the location-0 attribute is always bound to something
        "in vec3 GeoNormal;\n"
		"in vec3 Normal;\n"
		"in vec4 Color;\n"
        "in vec4 ControlColor;"
		"in vec2 TexCoord;\n"
		"out vec3 position;\n"
        "out vec3 geoNormal;\n"
		"out vec3 shadingNormal;\n"
		"out vec4 color;\n"
        "out vec4 controlColor;\n"
		"out vec2 texCoord;\n"
		"void main() {\n"
		"	gl_Position = object_to_clip * Position;\n"
		"	position = object_to_light * Position;\n"
		"	shadingNormal = normal_to_light * Normal;\n"
        "   geoNormal = GeoNormal;\n"
        //"   shadingNormal = geoNormal;\n"
        //"   geoNormal = shadingNormal;\n"
		"	color = Color;\n"
        "   controlColor = ControlColor;\n"
		"	texCoord = TexCoord;\n"
        "   vec2 pixel_size = clip_units_per_pixel * gl_Position.w;\n"
        "   vec2 voffset = sin(time*speed+(gl_Position.x+gl_Position.y+gl_Position.z)*frequency)*tremor_amount*pixel_size;\n"
        "   float a = 0.8f;\n"
        "   vec3 viewDir = normalize(viewPos-position);\n"
        "   gl_Position = gl_Position+vec4(voffset,0, 0)*(1.0f-a*dot(viewDir,geoNormal));\n"
		"}\n"
		,
		"#version 330\n"
		"uniform vec3 sun_direction;\n"
		"uniform vec3 sun_color;\n"
		"uniform vec3 sky_direction;\n"
		"uniform vec3 sky_color;\n"
        "uniform float dA;\n"
        "uniform float cangiante_variable;\n"
		"uniform float dilution_variable;\n"
        "uniform sampler2D tex;\n"
		"in vec3 position;\n"
        "in vec3 geoNormal;\n"
		"in vec3 shadingNormal;\n"
		"in vec4 color;\n"
        "in vec4 controlColor;\n"
		"in vec2 texCoord;\n"
		//"out vec4 fragColor;\n"
        "layout(location=0) out vec4 color_out;\n"
        "layout(location=1) out vec4 control_out;\n"
		"void main() {\n"
		"	vec3 total_light = vec3(0.0, 0.0, 0.0);\n"
		"	vec3 n = normalize(shadingNormal);\n"
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

		"	color_out = texture(tex, texCoord) * vec4(color.rgb*1.08f*total_light, 1.0);\n"
		"	control_out = controlColor;\n"

        //pixel shader stuff (dilution, pigment turbulence)
        "   float DA = 1.3f*(dot(sky_direction, geoNormal)+(dA-1.f))/dA+(dot(sun_direction, geoNormal)+(dA-1.f))/dA;\n"
        //"   float DA = (dot(light_direction, geoNormal)+(dA-1.f))/dA;\n"
        //"   float DA = (dot(sky_direction, geoNormal)+(dA-1.f))/dA;\n"
        //TODO how is this supposed to cause a hue shift?
        "   vec4 cangiante = color_out+DA*cangiante_variable;\n"
        "   vec4 paper = vec4(1.f, 1.f, 1.f, 1.f);\n" //TODO maybe change later?
        "   color_out = dilution_variable*DA*(paper-cangiante)+cangiante;\n"
        "   float ctrl = control_out.a;\n"
        "   if(ctrl<0.5){\n"
        "       float exp = 3.f-(ctrl*4.f);\n"
        "       color_out.r = pow(color_out.r, exp);\n"
        "       color_out.g = pow(color_out.g, exp);\n"
        "       color_out.b = pow(color_out.b, exp);\n"
        "   }else{\n"
        "       color_out = (ctrl-0.5f)*2.f*(paper-color_out)+color_out;\n"
        "   }\n"
		"}\n"
	);
    object_to_clip_mat4 = glGetUniformLocation(program, "object_to_clip");
	object_to_light_mat4x3 = glGetUniformLocation(program, "object_to_light");
	normal_to_light_mat3 = glGetUniformLocation(program, "normal_to_light");

	time = glGetUniformLocation(program, "time");
	speed = glGetUniformLocation(program, "speed");
	frequency = glGetUniformLocation(program, "frequency");
    tremor_amount = glGetUniformLocation(program, "tremor_amount");
	clip_units_per_pixel =glGetUniformLocation(program, "clip_units_per_pixel");
	viewPos = glGetUniformLocation(program, "viewPos");
    dA = glGetUniformLocation(program, "dA");
    cangiante_variable = glGetUniformLocation(program, "cangiante_variable");
    dilution_variable = glGetUniformLocation(program, "dilution_variable");

	sun_direction_vec3 = glGetUniformLocation(program, "sun_direction");
	sun_color_vec3 = glGetUniformLocation(program, "sun_color");
	sky_direction_vec3 = glGetUniformLocation(program, "sky_direction");
	sky_color_vec3 = glGetUniformLocation(program, "sky_color");

	glUseProgram(program);

	GLuint tex_sampler2D = glGetUniformLocation(program, "tex");
	glUniform1i(tex_sampler2D, 0);

	glUseProgram(0);

	GL_ERRORS();
}

Load< SceneProgram > scene_program(LoadTagInit, [](){
	return new SceneProgram();
});
