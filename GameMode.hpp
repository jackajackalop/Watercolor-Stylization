#pragma once

#include "Mode.hpp"
#include "Load.hpp"

#include "MeshBuffer.hpp"
#include "GL.hpp"

#include <SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <chrono>

#include <vector>

// The 'GameMode' mode is the main gameplay mode:

struct GameMode : public Mode {
	GameMode();
	virtual ~GameMode();

	//handle_event is called when new mouse or keyboard events are received:
	// (note that this might be many times per frame or never)
	//The function should return 'true' if it handled the event.
	virtual bool handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) override;

	//update is called at the start of a new frame, after events are handled:
	virtual void update(float elapsed) override;

	//draw is called after update:
	virtual void draw(glm::uvec2 const &drawable_size) override;
    void draw_scene(GLuint* control_tex, GLuint* color_tex, GLuint* depth_tex);
    void draw_mrt_blur(GLuint color_tex, GLuint depth_tex, GLuint control_tex,
                GLuint* temp_tex, GLuint* blurred_tex, GLuint* bleeded_tex);
    void draw_surface(GLuint paper_tex, GLuint normal_map_tex,
                        GLuint* surface_tex);
    void draw_stylization(GLuint control_tex, GLuint color_tex,
                        GLuint surface_tex, GLuint blurred_tex,
                        GLuint bleeded_tex, GLuint* final_tex);

	float camera_spin = 0.0f;
	float spot_spin = 0.0f;
};
