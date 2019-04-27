#pragma once

#include "Mode.hpp"
#include "Load.hpp"

#include "MeshBuffer.hpp"
#include "GL.hpp"

#include <SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <chrono>
#include <stdio.h>


#include <vector>
#include <string>

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
    void get_weights();
    void draw_scene(GLuint* control_tex_, GLuint* color_tex_,
            GLuint* depth_tex_);
    void draw_mrt_blur(GLuint color_tex, GLuint control_tex, GLuint depth_tex,
                GLuint* blur_temp_tex_, GLuint* bleed_temp_tex_,
                GLuint* control_temp_tex_, GLuint* final_control_tex_,
                GLuint* blurred_tex_, GLuint* bleeded_tex_);
    void draw_surface(GLuint paper_tex, GLuint *surface_tex_);
    void draw_stylization(GLuint final_control_tex, GLuint color_tex,
                        GLuint surface_tex, GLuint blurred_tex,
                        GLuint bleeded_tex, GLuint* final_tex_);
    void write_png(const char *filename);

    glm::quat camera_rot =  glm::angleAxis(glm::radians(0.0f),
            glm::vec3(1.0f, 0.0f, 0.0f));
    float yaw = 0.0;
    float pitch = 0.0;
    float weights[20];
};
