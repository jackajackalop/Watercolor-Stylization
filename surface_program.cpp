#include "surface_program.hpp"

#include "compile_program.hpp"
#include "gl_errors.hpp"

SurfaceProgram::SurfaceProgram() {
	program = compile_program(
		"#version 330\n"
		"void main() {\n"
        "   gl_Position = vec4(4*(gl_VertexID & 1) -1, 2 * (gl_VertexID &2) -1, 0.0, 1.0);"
		"}\n"
		,
		"#version 330\n"
		"uniform sampler2D paper_tex;\n"
        "uniform sampler2D normal_map_tex;\n"
        "layout(location=0) out vec4 surface_out;\n"
		"void main() {\n"
        "   vec2 texCoord = gl_FragCoord.xy/textureSize(paper_tex, 0); \n"
		"	vec4 paperColor = texture(paper_tex, texCoord);\n"
        "   float paperHeight = paperColor.r; \n"
        "   vec4 normalColor = texture(normal_map_tex, texCoord);"
        "   vec3 xdirection = normalize(vec3(1.0 ,0.0, dFdx(paperHeight)));\n"
        "   vec3 ydirection = normalize(vec3(0.0, 1.0, dFdy(paperHeight)));\n"
        "   vec3 n = normalize(cross(xdirection, ydirection));\n"
        "   vec3 l = normalize(vec3(1.0, 1.0, 1.0));"
        "   float nl = (dot(n,l)+1.0)/2.0;\n"
        "   nl = mix(0.65, 1.1, nl); \n"
        "   surface_out = vec4(paperHeight, 0.5*n.xy+0.5, nl);"
		"}\n"
	);
	glUseProgram(program);

    glUniform1i(glGetUniformLocation(program, "paper_tex"), 0);
    glUniform1i(glGetUniformLocation(program, "normal_map_tex"), 1);

	glUseProgram(0);

	GL_ERRORS();
}

Load< SurfaceProgram > surface_program(LoadTagInit, [](){
	return new SurfaceProgram();
});
