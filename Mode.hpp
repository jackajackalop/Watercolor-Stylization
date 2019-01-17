#pragma once

#include <SDL.h>
#include <glm/glm.hpp>

#include <memory>

class Mode : public std::enable_shared_from_this< Mode > {
public:
	virtual ~Mode() { }

	//handle_event is called when new mouse or keyboard events are received:
	// (note that this might be many times per frame or never)
	//The function should return 'true' if it handled the event.
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) { return false; }

	//update is called at the start of a new frame, after events are handled:
	// 'elapsed' is time in seconds since the last call to 'update'
	virtual void update(float elapsed) { }

	//draw is called after update:
	virtual void draw(glm::uvec2 const &drawable_size) = 0;

	//Mode::current is the Mode to which events are dispatched.
	// use 'set_current' to change the current Mode (e.g., to switch to a menu)
	static std::shared_ptr< Mode > current;
	static void set_current(std::shared_ptr< Mode > const &);
};

