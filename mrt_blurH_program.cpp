#include "mrt_blurH_program.hpp"

#include "compile_program.hpp"
#include "gl_errors.hpp"

MRTBlurHProgram::MRTBlurHProgram() {
	program = compile_program(
		"#version 330\n"
		"void main() {\n"
        "   gl_Position = vec4(4*(gl_VertexID & 1) -1, 2 * (gl_VertexID &2) -1, 0.0, 1.0);"
		"}\n"
		,
		"#version 330\n"
		"uniform sampler2D color_tex;\n"
        "uniform sampler2D control_tex;\n"
        "uniform sampler2D depth_tex;\n"
        "layout(location=0) out vec4 blurred_out;\n"
        "layout(location=1) out vec4 bleeded_out;\n"
		"void main() {\n"
		"	vec4 fragColor = texelFetch(color_tex, ivec2(gl_FragCoord.xy), 0);\n"
        "   bleeded_out = vec4(fragColor.r, fragColor.g, fragColor.b, 1.0);\n"

        //gaussian blur
        //https://learnopengl.com/Advanced-Lighting/Bloom
        "   float weight[5] = float[](0.227027f, 0.1945946f, 0.1216216f, 0.054054f, 0.016216f);\n"
        "   blurred_out = fragColor*weight[0];\n"
        "   for(int i = 1; i<5; ++i){\n"
        "       blurred_out += texelFetch(color_tex, ivec2(gl_FragCoord.xy+vec2(i, 0)), 0)*weight[i];\n"
        "       blurred_out += texelFetch(color_tex, ivec2(gl_FragCoord.xy-vec2(i, 0)), 0)*weight[i];\n"
        "   }\n"
        //"   blurred_out = vec4(1, 0, 1, 1);\n"
		"}\n"

	);
	glUseProgram(program);

    glUniform1i(glGetUniformLocation(program, "color_tex"), 0);
    glUniform1i(glGetUniformLocation(program, "control_tex"), 1);
    glUniform1i(glGetUniformLocation(program, "depth_tex"), 2);

	glUseProgram(0);

	GL_ERRORS();
}

Load< MRTBlurHProgram > mrt_blurH_program(LoadTagInit, [](){
	return new MRTBlurHProgram();
});
