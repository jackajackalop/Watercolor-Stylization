#include "mrt_blur_program.hpp"

#include "compile_program.hpp"
#include "gl_errors.hpp"

//weights thanks to http://dev.theomader.com/gaussian-kernel-calculator/
//please pass in "ivec2(0, i)" or "ivec2(i, 0)"
#define BLUR_SHADER(OFFSET) \
        "#version 330\n" \
		"uniform sampler2D blur_color_tex;\n" \
        "uniform sampler2D bleed_color_tex;\n" \
        "uniform sampler2D control_tex;\n" \
        "uniform sampler2D depth_tex;\n" \
        "uniform float depth_threshold;\n" \
        "uniform int blur_amount; \n" \
        "uniform float weights[20];\n"\
        "layout(location=0) out vec4 blurred_out;\n"\
        "layout(location=1) out vec4 bleeded_out;\n"\
        "layout(location=2) out vec4 control_out;\n"\
        "#define OFFSET " OFFSET " \n" \
		"void main() {\n"\
        "   #define RADIUS " STR(blur_amount) "\n"\
		"	vec4 fragColor = texelFetch(blur_color_tex, ivec2(gl_FragCoord.xy), 0);\n"\
        "//gaussian blur\n"\
        "//https://learnopengl.com/Advanced-Lighting/Bloom\n"\
        "   blurred_out = fragColor*weights[0];\n"\
        "   for(int i = 1; i<RADIUS; ++i){\n"\
        "       blurred_out += texelFetch(blur_color_tex, ivec2(gl_FragCoord.xy)+OFFSET, 0)*weights[i];\n"\
        "       blurred_out += texelFetch(blur_color_tex, ivec2(gl_FragCoord.xy)-OFFSET, 0)*weights[i];\n"\
        "   }\n"\
        "//4D joint bilateral blur\n"\
        "//http://dev.theomader.com/gaussian-kernel-calculator/\n"\
        "   float weight21[21] = float[](0.043959f, 0.045015f, 0.045982f, "\
                    "0.046852f, 0.047619f, 0.048278f, 0.048825f, 0.049254f, "\
                    "0.049562f, 0.049748f, 0.049812f, 0.049748f, 0.049562f, "\
                    "0.049254f, 0.048825f, 0.048278f, 0.047619f, 0.046852f, "\
                    "0.045982f, 0.045015f, 0.043959f);\n"\
        "   bleeded_out = vec4(0.0, 0.0, 0.0, 1.0);\n"\
        "   vec4 control_in = texelFetch(control_tex, ivec2(gl_FragCoord.xy), 0);\n"\
        "   float ctrlx=control_in.b;\n"\
		"   for(int i = -10; i<=10; i++){\n"\
        "       bool bleed;\n"\
        "       float ctrlxi = texelFetch(control_tex, ivec2(gl_FragCoord.xy)+OFFSET, 0).b;\n"\
        "       if (ctrlx>0 || ctrlxi>0) {\n"\
        "           bleed = false;\n"\
        "           float zx = texelFetch(depth_tex, ivec2(gl_FragCoord.xy), 0).z;\n"\
        "           float zxi = texelFetch(depth_tex, ivec2(gl_FragCoord.xy)+OFFSET, 0).z;\n"\
        "           if ((zx-depth_threshold) > zxi){ //source is behind\n"\
        "               if (ctrlxi>0) bleed = true;\n"\
        "           } else {\n //source is not behind\n"\
        "               //so the paper says to do this, but that seems wrong" \
        "               //if (ctrlx>0) bleed = true;\n"\
        "           }\n"\
        "           if (bleed) {\n"\
        "               bleeded_out = bleeded_out+texelFetch(bleed_color_tex, ivec2(gl_FragCoord.xy)+OFFSET, 0)*weight21[i+10];\n"\
        "           } else {\n"\
        "               bleeded_out = bleeded_out+texelFetch(bleed_color_tex, ivec2(gl_FragCoord.xy), 0)*weight21[i+10];\n"\
        "           } \n"\
        "       } else {\n"\
        "           bleeded_out = bleeded_out+texelFetch(bleed_color_tex, ivec2(gl_FragCoord.xy), 0)*weight21[i+10];\n"\
        "       }\n"\
        "//TODO i dont really understand whats going on here"\
        "       control_out = control_in;\n"\
        "       if (bleed) control_out.b = 0.0;\n"\
        "   }\n"\
        "   bleeded_out.a = 1.0;\n"\
        "}\n" \


MRTBlurHProgram::MRTBlurHProgram() {
	program = compile_program(
		"#version 330\n"
		"void main() {\n"
        "   gl_Position = vec4(4*(gl_VertexID & 1) -1, 2 * (gl_VertexID &2) -1, 0.0, 1.0);"
		"}\n"
		,
        BLUR_SHADER("ivec2(i, 0)")
	);
	glUseProgram(program);

    depth_threshold = glGetUniformLocation(program, "depth_threshold");
    blur_amount = glGetUniformLocation(program, "blur_amount");
    weights = glGetUniformLocation(program, "weights");
    glUniform1i(glGetUniformLocation(program, "blur_color_tex"), 0);
    glUniform1i(glGetUniformLocation(program, "bleed_color_tex"), 0);
    glUniform1i(glGetUniformLocation(program, "control_tex"), 1);
    glUniform1i(glGetUniformLocation(program, "depth_tex"), 2);

	glUseProgram(0);

	GL_ERRORS();
}

MRTBlurVProgram::MRTBlurVProgram() {
	program = compile_program(
		"#version 330\n"
		"void main() {\n"
        "   gl_Position = vec4(4*(gl_VertexID & 1) -1, 2 * (gl_VertexID &2) -1, 0.0, 1.0);"
		"}\n"
		,
        BLUR_SHADER("ivec2(0, i)")
	);
	glUseProgram(program);

    depth_threshold = glGetUniformLocation(program, "depth_threshold");
    blur_amount = glGetUniformLocation(program, "blur_amount");
    weights = glGetUniformLocation(program, "weights");

    glUniform1i(glGetUniformLocation(program, "blur_color_tex"), 0);
    glUniform1i(glGetUniformLocation(program, "bleed_color_tex"), 1);
    glUniform1i(glGetUniformLocation(program, "control_tex"), 2);
    glUniform1i(glGetUniformLocation(program, "depth_tex"), 3);

	glUseProgram(0);

	GL_ERRORS();
}

Load< MRTBlurVProgram > mrt_blurV_program(LoadTagInit, [](){
	return new MRTBlurVProgram();
});
Load< MRTBlurHProgram > mrt_blurH_program(LoadTagInit, [](){
	return new MRTBlurHProgram();
});
