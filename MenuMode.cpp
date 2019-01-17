#include "MenuMode.hpp"

#include "Load.hpp"
#include "compile_program.hpp"
#include "draw_text.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <iostream>

//---------- resources ------------

GLint fade_program_color = -1;

Load< GLuint > fade_program(LoadTagInit, [](){
	GLuint *ret = new GLuint(compile_program(
		"#version 330\n"
		"void main() {\n"
		"	gl_Position = vec4(4 * (gl_VertexID & 1) - 1,  2 * (gl_VertexID & 2) - 1, 0.0, 1.0);\n"
		"}\n"
	,
		"#version 330\n"
		"uniform vec4 color;\n"
		"out vec4 fragColor;\n"
		"void main() {\n"
		"	fragColor = color;\n"
		"}\n"
	));

	fade_program_color = glGetUniformLocation(*ret, "color");

	return ret;
});

//vao that binds nothing:
Load< GLuint > empty_binding(LoadTagDefault, [](){
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	//empty vao has no attribute locations bound.
	glBindVertexArray(0);
	return new GLuint(vao);
});

//----------------------

bool MenuMode::handle_event(SDL_Event const &e, glm::uvec2 const &window_size) {
	if (e.type == SDL_KEYDOWN) {
		if (e.key.keysym.sym == SDLK_ESCAPE) {
			if (on_escape) {
				on_escape();
			} else {
				Mode::set_current(background);
			}
			return true;
		} else if (e.key.keysym.sym == SDLK_UP) {
			//find previous selectable thing that isn't selected:
			uint32_t old = selected;
			selected -= 1;
			while (selected < choices.size() && !choices[selected].on_select) --selected;
			if (selected >= choices.size()) selected = old;

			return true;
		} else if (e.key.keysym.sym == SDLK_DOWN) {
			//find next selectable thing that isn't selected:
			uint32_t old = selected;
			selected += 1;
			while (selected < choices.size() && !choices[selected].on_select) ++selected;
			if (selected >= choices.size()) selected = old;

			return true;
		} else if (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_SPACE) {
			if (selected < choices.size() && choices[selected].on_select) {
				choices[selected].on_select();
			}
			return true;
		}
	}
	return false;
}

void MenuMode::update(float elapsed) {
	bounce += elapsed / 0.7f;
	bounce -= std::floor(bounce);

	if (background) {
		background->update(elapsed * background_time_scale);
	}
}

void MenuMode::draw(glm::uvec2 const &drawable_size) {
	if (background && background_fade < 1.0f) {
		background->draw(drawable_size);

		glDisable(GL_DEPTH_TEST);
		if (background_fade > 0.0f) {
			glEnable(GL_BLEND);
			glBlendEquation(GL_FUNC_ADD);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glUseProgram(*fade_program);
			glUniform4fv(fade_program_color, 1, glm::value_ptr(glm::vec4(0.0f, 0.0f, 0.0f, background_fade)));
			glDrawArrays(GL_TRIANGLES, 0, 3);
			glUseProgram(0);
			glDisable(GL_BLEND);
		}
	}
	glDisable(GL_DEPTH_TEST);

	float total_height = 0.0f;
	for (auto const &choice : choices) {
		total_height += choice.height + 2.0f * choice.padding;
	}

	float select_bounce = std::abs(std::sin(bounce * 3.1515926f * 2.0f));

	float y = 0.5f * total_height;
	for (auto const &choice : choices) {
		y -= choice.padding;
		y -= choice.height;

		bool is_selected = (&choice - &choices[0] == selected);
		std::string label = choice.label;

		float s = choice.height * (1.0f / 3.0f);

		float width = text_width(label, 3.0f);

		if (is_selected) {
			float star_width = text_width("*", 3.0f);
			draw_text("*", glm::vec2(s * (-0.5f * width - select_bounce - star_width), y), s);
			draw_text(label, glm::vec2(s * (-0.5f * width), y), s);
			draw_text("*", glm::vec2(s * (0.5f * width + select_bounce), y), s);
		}

		y -= choice.padding;
	}

	glEnable(GL_DEPTH_TEST);
}
