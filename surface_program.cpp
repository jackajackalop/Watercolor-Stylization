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
        "layout(location=0) out vec4 surface_tex;\n"
		"void main() {\n"
		"	vec4 paperColor = texelFetch(paper_tex, ivec2(gl_FragCoord.xy), 0);\n"
        "   vec4 normalColor = texelFetch(normal_map_tex, ivec2(gl_FragCoord.xy), 0);"
        "   surface_tex = vec4(0.5*paperColor.r+0.5*normalColor.r, 0.5*paperColor.g+0.5*normalColor.g,0.5*paperColor.b+0.5*normalColor.b, 1.0);"
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
