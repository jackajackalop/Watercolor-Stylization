#include "depth_program.hpp"

#include "compile_program.hpp"

DepthProgram::DepthProgram() {
	program = compile_program(
		"#version 330\n"
		"uniform mat4 object_to_clip;\n"
		"layout(location=0) in vec4 Position;\n" //note: layout keyword used to make sure that the location-0 attribute is always bound to something
		"in vec3 Normal;\n" //DEBUG
		"out vec3 color;\n" //DEBUG
		"void main() {\n"
		"	gl_Position = object_to_clip * Position;\n"
		"	color = 0.5 + 0.5 * Normal;\n" //DEBUG
		"}\n"
		,
		"#version 330\n"
		"in vec3 color;\n" //DEBUG
		//"uniform vec4 color;\n"
		"out vec4 fragColor;\n"
		"void main() {\n"
		"	fragColor = vec4(color, 1.0);\n"
		"}\n"
	);

	object_to_clip_mat4 = glGetUniformLocation(program, "object_to_clip");
}

Load< DepthProgram > depth_program(LoadTagInit, [](){
	return new DepthProgram();
});
