
#include <SDL.h>
#include <iostream>
#include <stdexcept>

#define DO(TYPE, NAME) \
	PFNGL ## TYPE ## PROC gl ## NAME = NULL;
#include "gl_shims.hpp"
#undef DO
#undef GL_SHIMS_HPP

void init_gl_shims() {
	#define DO(TYPE, NAME) \
		gl ## NAME = (PFNGL ## TYPE ## PROC)SDL_GL_GetProcAddress("gl" #NAME); \
		if (!gl ## NAME) { \
			throw std::runtime_error("Error binding "  "gl" #NAME); \
		}
#include "gl_shims.hpp"
}
