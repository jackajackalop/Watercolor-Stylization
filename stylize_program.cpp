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
        "uniform float density_amount;\n"
        "layout(location=0) out vec4 final_out;\n"

        "vec4 pow_col(vec4 base, float exp){ \n"
        "   return vec4(pow(base.r,exp),pow(base.g, exp),pow(base.b, exp),1);\n"
        "}\n"

		"void main() {\n"
        "   vec4 surfaceColor = texelFetch(surface_tex, ivec2(gl_FragCoord.xy), 0);\n"
        //paper distortion..?
        "   float ctrl = texelFetch(control_tex, ivec2(gl_FragCoord.xy),0).r;"
        "   vec2 shift_amt = ctrl*surfaceColor.gb; \n"
        "   ivec2 shiftedCoord = ivec2(gl_FragCoord.xy+shift_amt);\n"

		"	vec4 controlColor = texelFetch(control_tex, shiftedCoord, 0);\n"
        "   vec4 colorColor = texelFetch(color_tex, shiftedCoord, 0);\n"
        "   vec4 blurredColor = texelFetch(blurred_tex, shiftedCoord, 0);\n"
        "   vec4 bleededColor = texelFetch(bleeded_tex, shiftedCoord, 0);\n"
        "   vec4 colorBleed = controlColor.b*(bleededColor-colorColor)+colorColor;\n"
        "   vec4 blurDif = (blurredColor-colorColor);\n"
        "   float maxVal = max(0.0,max(blurDif.r, max(blurDif.g, blurDif.b)));\n"
        "   float exp = 1.0+(1.0-controlColor.b)*maxVal*5.0; \n"
        "   vec4 edgeDarkening =pow_col(colorBleed, exp); \n"
        "   final_out = edgeDarkening; \n"
        "   vec4 saturation = edgeDarkening;\n"
        "   vec4 surface = texelFetch(surface_tex, shiftedCoord,0);\n"
        "   float paperHeight = surface.r;\n"
        "   vec2 offset = surface.gb*2.0-1.0;\n"
        "   float tint = surface.a;\n"
        "   float Piv = 0.5*(1.0-paperHeight);\n"
        "   ctrl = texelFetch(control_tex, shiftedCoord,0).g;"
        "   vec4 granulated = saturation*(saturation-ctrl*density_amount*Piv)+(1.0-saturation)*pow_col(saturation, 1.0+(ctrl*density_amount*Piv)); \n"
        //"   final_out = vec4(tint,tint, tint, 1.0); \n"
        "   final_out = granulated*tint;\n"
        //"   final_out = controlColor;\n"
		"}\n"
	);
	glUseProgram(program);

    glUniform1i(glGetUniformLocation(program, "color_tex"), 0);
    glUniform1i(glGetUniformLocation(program, "control_tex"), 1);
    glUniform1i(glGetUniformLocation(program, "blurred_tex"), 2);
    glUniform1i(glGetUniformLocation(program, "bleeded_tex"), 3);
    glUniform1i(glGetUniformLocation(program, "surface_tex"), 4);

    density_amount = glGetUniformLocation(program, "density_amount");

	glUseProgram(0);

	GL_ERRORS();
}

Load< StylizeProgram > stylize_program(LoadTagInit, [](){
	return new StylizeProgram();
});
