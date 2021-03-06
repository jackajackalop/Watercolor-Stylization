//Mode.hpp declares the "Mode::current" static member variable, which is used to decide where event-handling, updating, and drawing events go:
#include "Mode.hpp"

//Load.hpp is included because of the call_load_functions() call:
#include "Load.hpp"

//The 'GameMode' mode plays the game:
#include "GameMode.hpp"

//The 'Sound' header has functions for managing sound:
#include "Sound.hpp"

//GL.hpp will include a non-namespace-polluting set of opengl prototypes:
#include "GL.hpp"

//Includes for libSDL:
#include <SDL.h>

//...and for glm:
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

//...and for c++ standard library functions:
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <fstream>
#include <memory>
#include <algorithm>
#include <string>

#include "parameters.hpp"

extern std::string file;
extern bool pic_mode;
int main(int argc, char **argv) {
#ifdef _WIN32
	try {
#endif
	struct {
		std::string title = "Watercolor Stylization";
		glm::uvec2 size = glm::uvec2(2420,1311);//1280, 800);//1329, 1329);
	} config;

    if(argc>=2 && argv[1][0]!='-'){
        file = argv[1];
    }

    //parsing input parameters
    //-time = elapsed time
    //-speed = speed of hand tremors
    //-frequency = frequency of hand tremors
    //-dA =  dilute area variable
    //-cangiante = cangiante variable
    //-dilution = dilution_variable
    //-density = density amount
    //-depth = depth threshold
    //-blur = blur amount
    //-bleed = turns off and on bleed (0 is false, every other int is true)
    //-distortion = turns off and on paper distortion (0 for false)
    //-show = show
    //-file = filename
    //-save = turns on pic_mode and sets filename
    int start = (argc%2==0 ? 2 : 1);
    for(int i = start; i<argc-1; i+=2){
        if(strcmp(argv[i], "-time")==0){
            Parameters::elapsed_time = atof(argv[i+1]);
        }else if(strcmp(argv[i], "-speed")==0){
            Parameters::speed = atof(argv[i+1]);
        }else if(strcmp(argv[i], "-frequency")==0){
            Parameters::frequency = atof(argv[i+1]);
        }else if(strcmp(argv[i], "-dA") == 0){
            Parameters::dA = atof(argv[i+1]);
        }else if(strcmp(argv[i],"-cangiante") == 0){
            Parameters::cangiante_variable = atof(argv[i+1]);
        }else if(strcmp(argv[i],"-dilution") == 0){
            Parameters::dilution_variable = atof(argv[i+1]);
        }else if(strcmp(argv[i],"-density") == 0){
            Parameters::density_amount = atof(argv[i+1]);
        }else if(strcmp(argv[i],"-depth") == 0){
            Parameters::depth_threshold = atof(argv[i+1]);
        }else if(strcmp(argv[i],"-blur") == 0){
            Parameters::blur_amount = atof(argv[i+1]);
        }else if(strcmp(argv[i],"-bleed") == 0){
            Parameters::bleed = atoi(argv[i+1]);
        }else if(strcmp(argv[i],"-distortion") == 0){
            Parameters::distortion = atoi(argv[i+1]);
        }else if(strcmp(argv[i],"-show") == 0){
            Parameters::show = atoi(argv[i+1]);
        }else if(strcmp(argv[i], "-file") == 0){
            Parameters::filename = argv[i+1];
        }else if(strcmp(argv[i], "-save") == 0){
            Parameters::filename = argv[i+1];
            pic_mode = true;
        }

    }
	/*
	//----- start connection to server ----
	if (argc != 3) {
		std::cout << "Usage:\n\t./client <host> <port>" << std::endl;
		return 1;
	}

	Client client(argv[1], argv[2]);
	*/

	//------------  initialization ------------

	//Initialize SDL library:
	SDL_Init(SDL_INIT_VIDEO);

	//Ask for an OpenGL context version 3.3, core profile, enable debug:
	SDL_GL_ResetAttributes();
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

	//create window:
	SDL_Window *window = SDL_CreateWindow(
		config.title.c_str(),
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		config.size.x, config.size.y,
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
	);

	//prevent exceedingly tiny windows when resizing:
	SDL_SetWindowMinimumSize(window, 100, 100);

	if (!window) {
		std::cerr << "Error creating SDL window: " << SDL_GetError() << std::endl;
		return 1;
	}

	//Create OpenGL context:
	SDL_GLContext context = SDL_GL_CreateContext(window);

	if (!context) {
		SDL_DestroyWindow(window);
		std::cerr << "Error creating OpenGL context: " << SDL_GetError() << std::endl;
		return 1;
	}

	#ifdef _WIN32
	//On windows, load OpenGL extensions:
	init_gl_shims();
	#endif

	//Set VSYNC + Late Swap (prevents crazy FPS):
	if (SDL_GL_SetSwapInterval(-1) != 0) {
		std::cerr << "NOTE: couldn't set vsync + late swap tearing (" << SDL_GetError() << ")." << std::endl;
		if (SDL_GL_SetSwapInterval(1) != 0) {
			std::cerr << "NOTE: couldn't set vsync (" << SDL_GetError() << ")." << std::endl;
		}
	}

	//Hide mouse cursor (note: showing can be useful for debugging):
	//SDL_ShowCursor(SDL_DISABLE);

	//------------ init sound output --------------
	Sound::init();

	//------------ load assets --------------

	call_load_functions();

	//------------ create game mode + make current --------------

	Mode::set_current(std::make_shared< GameMode >(/*client*/));

	//------------ main loop ------------

	//the window created above is resizable; this inline function will be
	//called whenever the window is resized, and will update the window_size
	//and drawable_size variables:
	glm::uvec2 window_size; //size of window (layout pixels)
	glm::uvec2 drawable_size; //size of drawable (physical pixels)
	//On non-highDPI displays, window_size will always equal drawable_size.
	auto on_resize = [&](){
		int w,h;
		SDL_GetWindowSize(window, &w, &h);
		window_size = glm::uvec2(w, h);
		SDL_GL_GetDrawableSize(window, &w, &h);
		drawable_size = glm::uvec2(w, h);
		glViewport(0, 0, drawable_size.x, drawable_size.y);
	};
	on_resize();

	//This will loop until the current mode is set to null:
	while (Mode::current) {
		//every pass through the game loop creates one frame of output
		//  by performing three steps:

		{ //(1) process any events that are pending
			static SDL_Event evt;
			while (SDL_PollEvent(&evt) == 1) {
				//handle resizing:
				if (evt.type == SDL_WINDOWEVENT && evt.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
					on_resize();
				}
				//handle input:
				if (Mode::current && Mode::current->handle_event(evt, window_size)) {
					// mode handled it; great
				} else if (evt.type == SDL_QUIT) {
					Mode::set_current(nullptr);
					break;
				}
			}
			if (!Mode::current) break;
		}

		{ //(2) call the current mode's "update" function to deal with elapsed time:
			auto current_time = std::chrono::high_resolution_clock::now();
			static auto previous_time = current_time;
			float elapsed = std::chrono::duration< float >(current_time - previous_time).count();
			previous_time = current_time;

			//if frames are taking a very long time to process,
			//lag to avoid spiral of death:
			elapsed = std::min(0.1f, elapsed);

			Mode::current->update(elapsed);
			if (!Mode::current) break;
		}

		{ //(3) call the current mode's "draw" function to produce output:
			//clear the depth+color buffers and set some default state:
			glClearColor(0.5, 0.5, 0.5, 0.0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glEnable(GL_DEPTH_TEST);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			Mode::current->draw(drawable_size);
		}

		//Finally, wait until the recently-drawn frame is shown before doing it all again:
		SDL_GL_SwapWindow(window);
	}


	//------------  teardown ------------

	SDL_GL_DeleteContext(context);
	context = 0;

	SDL_DestroyWindow(window);
	window = NULL;

	return 0;

#ifdef _WIN32
	} catch (std::exception const &e) {
		std::cerr << "Unhandled exception:\n" << e.what() << std::endl;
		return 1;
	} catch (...) {
		std::cerr << "Unhandled exception (unknown type)." << std::endl;
		throw;
	}
#endif
}
