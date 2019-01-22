#pragma once

#include "Mode.hpp"
#include "Load.hpp"

#include "MeshBuffer.hpp"
#include "GL.hpp"

#include <SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

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
    void draw_scene(Load<GLuint>* control_tex, Load<GLuint>* color_tex,
        Load<GLuint>* depth_tex);
    void draw_mrt_blur(Load<GLuint> color_tex, Load<GLuint> depth_tex,
                                                    Load<GLuint> conrol_tex,
            Load<GLuint>* blurred_tex, Load<GLuint>* bleeded_tex);
    void draw_surface(Load<GLuint> paper_tex, Load<GLuint> normal_map_tex,
            Load<GLuint>* surface_tex);
    void draw_stylization(Load<GLuint> control_tex, Load<GLuint> surface_tex,
            Load<GLuint> blurred_tex, Load<GLuint> bleeded_tex,
            Load<GLuint>* depth_tex);

	float camera_spin = 0.0f;
	float spot_spin = 0.0f;
};
