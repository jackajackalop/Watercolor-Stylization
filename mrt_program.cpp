#include "mrt_program.hpp"

#include "compile_program.hpp"
#include "gl_errors.hpp"

MRTProgram::MRTProgram() {
	program = compile_program(
		"#version 330\n"
		"void main() {\n"
        "   gl_Position = vec4(4*(gl_VertexID & 1) -1, 2 * (gl_VertexID &2) -1, 0.0, 1.0);"
		"}\n"
		,
		"#version 330\n"
		"uniform sampler2D control_tex;\n"
        "uniform sampler2D depth_tex;\n"
        "uniform sampler2D color_tex;\n"
        "layout(location=0) out vec4 bleeded_tex;\n"
        "layout(location=1) out vec4 blurred_tex;\n"
		"void main() {\n"
		"	vec4 fragColor = texelFetch(control_tex, ivec2(gl_FragCoord.xy), 0);\n"
        "   bleeded_tex = vec4(fragColor.r, fragColor.g*1.5, fragColor.b, 1.0);"
        "   blurred_tex = vec4(fragColor.r, fragColor.g, fragColor.b*1.5, 1.0);"
		"}\n"
	);
	glUseProgram(program);

    glUniform1i(glGetUniformLocation(program, "control_tex"), 0);
    glUniform1i(glGetUniformLocation(program, "depth_tex"), 1);
    glUniform1i(glGetUniformLocation(program, "color_tex"), 2);

	glUseProgram(0);

	GL_ERRORS();
}

Load< MRTProgram > mrt_program(LoadTagInit, [](){
	return new MRTProgram();
});
