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
        "uniform bool bleed; \n"
        "uniform bool distortion; \n "
        "layout(location=0) out vec4 final_out;\n"

        //calculates a vec4 that has each of its rgb components exponentiated
        "vec4 pow_col(vec4 base, float exp){ \n"
        "   return vec4(pow(base.r,exp),pow(base.g, exp),pow(base.b, exp),1);\n"
        "}\n"

        //calculates the max value of the rgb components of a vec4
        "float max_col(vec4 col){ \n"
        "   return max(0.0, max(col.r, max(col.g, col.b))); \n"
        "} \n"

		"void main() {\n"
        "   vec4 surfaceColor = texelFetch(surface_tex, ivec2(gl_FragCoord.xy), 0);\n"
        //paper distortion
        "   float ctrl = texelFetch(control_tex, ivec2(gl_FragCoord.xy),0).r;\n"
        "   vec2 shift_amt = surfaceColor.gb; \n"
        "   if(!distortion) shift_amt = vec2(0.0, 0.0); \n"

        //just getting all the values from each texture
        "   ivec2 shiftedCoord = ivec2(gl_FragCoord.xy+shift_amt);\n"
		"	vec4 controlColor = texelFetch(control_tex, shiftedCoord, 0);\n"
        "   vec4 colorColor = texelFetch(color_tex, shiftedCoord, 0);\n"
        "   vec4 blurredColor = texelFetch(blurred_tex, shiftedCoord, 0);\n"
        "   vec4 bleededColor = texelFetch(bleeded_tex, shiftedCoord, 0);\n"
        "   if(!bleed) bleededColor = colorColor; \n"

        //color bleeding
        "   vec4 colorBleed = controlColor.b*(bleededColor-colorColor)+colorColor;\n"

        //edge darkening
        "   vec4 blurDif = (blurredColor-colorColor);\n"
        "   float maxVal = max_col(blurDif);\n"
        "   float exp = 1.0+(1.0-controlColor.b)*maxVal*5.0; \n"
        "   vec4 edgeDarkening =pow_col(colorBleed, exp); \n"

        //paper granulation
        "   vec4 saturation = edgeDarkening;\n"
        "   vec4 surface = texelFetch(surface_tex, shiftedCoord,0);\n"
        "   float paperHeight = surface.r;\n"
        "   vec2 offset = surface.gb*2.0-1.0;\n"
        "   float tint = surface.a;\n"
        "   float Piv = 0.5*(1.0-paperHeight);\n"
        "   ctrl = texelFetch(control_tex, shiftedCoord,0).g;"
        "   vec4 granulated = saturation*(saturation-ctrl*density_amount*Piv)+(1.0-saturation)*pow_col(saturation, 1.0+(ctrl*density_amount*Piv)); \n"
        "   final_out = granulated*tint;\n"
        "   final_out.a = 1.0;\n"
		"}\n"
	);
	glUseProgram(program);

    glUniform1i(glGetUniformLocation(program, "color_tex"), 0);
    glUniform1i(glGetUniformLocation(program, "control_tex"), 1);
    glUniform1i(glGetUniformLocation(program, "blurred_tex"), 2);
    glUniform1i(glGetUniformLocation(program, "bleeded_tex"), 3);
    glUniform1i(glGetUniformLocation(program, "surface_tex"), 4);

    density_amount = glGetUniformLocation(program, "density_amount");
    bleed = glGetUniformLocation(program, "bleed");
    distortion = glGetUniformLocation(program, "distortion");

	glUseProgram(0);

	GL_ERRORS();
}

Load< StylizeProgram > stylize_program(LoadTagInit, [](){
	return new StylizeProgram();
});
