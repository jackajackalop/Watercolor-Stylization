#pragma once

#include "Mode.hpp"

#include <functional>
#include <vector>
#include <string>

struct MenuMode : public Mode {
	virtual ~MenuMode() { }

	virtual bool handle_event(SDL_Event const &event, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	struct Choice {
		Choice(std::string const &label_, std::function< void() > on_select_ = nullptr) : label(label_), on_select(on_select_) { }
		std::string label;
		std::function< void() > on_select;
		//height / padding give item height and padding relative to a screen of height 2:
		float height = 0.1f;
		float padding = 0.01f;
	};
	std::vector< Choice > choices;
	uint32_t selected = 0;
	float bounce = 0.0f;

	//called when user presses 'escape':
	// (note: if not defined, menumode will Mode::set_current(background).)
	std::function< void() > on_escape;

	//will render this mode in the background if not null:
	std::shared_ptr< Mode > background;
	float background_time_scale = 1.0f;
	float background_fade = 0.5f;
};
