#pragma once

#include <glm/glm.hpp>

#include <string>

//Helper functions to draw text:
//This version draws relative to a [-aspect,aspect]x[-1,1] screen.
// the 'anchor' gives the bottom left of the first character.
void draw_text(std::string const &text, glm::vec2 const &anchor, float height, glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

//This version uses an arbitrary matrix transformation on characters of height 1.0f anchored at (0,0):
void draw_text(std::string const &text, glm::mat4 const &transform, glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

//compute the width drawn by 'draw_text' for a string:
float text_width(std::string const &text, float height);
