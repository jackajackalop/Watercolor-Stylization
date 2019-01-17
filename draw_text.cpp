#include "draw_text.hpp"

#include "GL.hpp"
#include "Load.hpp"
#include "MeshBuffer.hpp"
#include "data_path.hpp"
#include "compile_program.hpp"

#include <glm/gtc/type_ptr.hpp>

//------------ resources ------------
Load< MeshBuffer > text_meshes(LoadTagInit, [](){
	return new MeshBuffer(data_path("menu.p"));
});

//font metrics for "text_meshes":
const constexpr float char_height = 3.0f;

inline float char_width(char a) {
	if (a == 'I') return 1.0f;
	else if (a == 'L') return 2.0f;
	else if (a == 'M' || a == 'W') return 4.0f;
	else return 3.0f;
};

inline float char_spacing(char a, char b) {
	return 1.0f;
}

//Uniform locations in text_program:
GLint text_program_mvp_mat4 = -1;
GLint text_program_color_vec4 = -1;

Load< GLuint > text_program(LoadTagInit, [](){
	GLuint *ret = new GLuint(compile_program(
		"#version 330\n"
		"uniform mat4 mvp;\n"
		"in vec4 Position;\n"
		"void main() {\n"
		"	gl_Position = mvp * Position;\n"
		"}\n"
	,
		"#version 330\n"
		"uniform vec4 color;\n"
		"out vec4 fragColor;\n"
		"void main() {\n"
		"	fragColor = color;\n"
		"}\n"
	));

	text_program_mvp_mat4 = glGetUniformLocation(*ret, "mvp");
	text_program_color_vec4 = glGetUniformLocation(*ret, "color");

	return ret;
});

//Binding for using text_program on text_meshes:
Load< GLuint > text_meshes_for_text_program(LoadTagDefault, [](){
	return new GLuint(text_meshes->make_vao_for_program(*text_program));
});

//----------------------


void draw_text(std::string const &text, glm::vec2 const &anchor, float height, glm::vec4 color) {
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	float aspect = viewport[2] / float(viewport[3]);

	draw_text(text,
		glm::mat4(
			height / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, height, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			anchor.x / aspect, anchor.y, 0.0f, 1.0f
		), color);
}

void draw_text(std::string const &text, glm::mat4 const &transform, glm::vec4 color) {
	glUseProgram(*text_program);
	glBindVertexArray(*text_meshes_for_text_program);

	float x = 0.0f;
	for (uint32_t i = 0; i < text.size(); ++i) {
		if (i > 0) x += char_spacing(text[i-1], text[i]);
		if (text[i] != ' ') {
			float s = 1.0f / char_height;
			glm::mat4 mvp = transform * glm::mat4(
				glm::vec4(s, 0.0f, 0.0f, 0.0f),
				glm::vec4(0.0f, s, 0.0f, 0.0f),
				glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
				glm::vec4(s * x, 0.0f, 0.0f, 1.0f)
			);
			glUniformMatrix4fv(text_program_mvp_mat4, 1, GL_FALSE, glm::value_ptr(mvp));
			glUniform4fv(text_program_color_vec4, 1, glm::value_ptr(color));

			MeshBuffer::Mesh const &mesh = text_meshes->lookup(text.substr(i,1));
			glDrawArrays(GL_TRIANGLES, mesh.start, mesh.count);
		}

		x += char_width(text[i]);
	}


	glBindVertexArray(0);
	glUseProgram(0);
}

float text_width(std::string const &text, float height) {
	float width = 0.0f;
	for (uint32_t i = 0; i < text.size(); ++i) {
		width += char_width(text[i]);
		if (i > 0) width += char_spacing(text[i-1], text[i]);
	}
	return width * (height / char_height);
}
