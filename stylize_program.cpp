#include "stylize_program.hpp"

#include "compile_program.hpp"
#include "gl_errors.hpp"

StylizeProgram::StylizeProgram() {
	program = compile_program(
		"#version 330\n"
		"void main() {\n"
        "   gl_Position = vec4(4*(gl_VertexID & 1) -1, 2 * (gl_VertexID &2) -1, 0.0, 1.0);"
		"}\n"
		,
		"#version 330\n"
		"uniform sampler2D color_tex;\n"
        "uniform sampler2D control_tex;\n"
        "uniform sampler2D blurred_tex;\n"
        "uniform sampler2D bleeded_tex;\n"
        "uniform sampler2D surface_tex;\n"
        "layout(location=0) out vec4 final_out;\n"
		"void main() {\n"
		"	vec4 controlColor = texelFetch(control_tex, ivec2(gl_FragCoord.xy), 0);\n"
        "   vec4 colorColor = texelFetch(color_tex, ivec2(gl_FragCoord.xy), 0);\n"
        "   vec4 blurredColor = texelFetch(blurred_tex, ivec2(gl_FragCoord.xy), 0);\n"
        "   vec4 bleededColor = texelFetch(bleeded_tex, ivec2(gl_FragCoord.xy), 0);\n" //TODO this is supposed to be upsampled??
        "   vec4 surfaceColor = texelFetch(surface_tex, ivec2(gl_FragCoord.xy), 0);\n"
        "   vec4 colorBleed = controlColor.b*(bleededColor-colorColor)+colorColor;\n"
        "   vec4 blurDif = blurredColor-colorColor;\n"
        "   float maxVal = max(blurDif.r, max(blurDif.g, max(blurDif.b, blurDif.a)));\n"
        "   float exp = 1+controlColor.b*maxVal; \n"
        "   vec4 edgeDarkening = vec4(pow(colorBleed.r, exp), pow(colorBleed.g, exp), pow(colorBleed.b, exp), colorBleed.a); \n"
        "   final_out = colorBleed; \n"
        //"   final_out = controlColor*0.2+colorColor*0.2+blurredColor*0.2+bleededColor*0.3+surfaceColor*0.2;\n"
		"}\n"
	);
	glUseProgram(program);

    glUniform1i(glGetUniformLocation(program, "color_tex"), 0);
    glUniform1i(glGetUniformLocation(program, "control_tex"), 1);
    glUniform1i(glGetUniformLocation(program, "blurred_tex"), 2);
    glUniform1i(glGetUniformLocation(program, "bleeded_tex"), 3);
    glUniform1i(glGetUniformLocation(program, "surface_tex"), 4);

	glUseProgram(0);

	GL_ERRORS();
}

Load< StylizeProgram > stylize_program(LoadTagInit, [](){
	return new StylizeProgram();
});
