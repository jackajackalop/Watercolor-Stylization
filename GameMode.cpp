#include "GameMode.hpp"

#include "MenuMode.hpp"
#include "Load.hpp"
#include "MeshBuffer.hpp"
#include "Scene.hpp"
#include "gl_errors.hpp" //helper for dumpping OpenGL error messages
#include "check_fb.hpp" //helper for checking currently bound OpenGL framebuffer
#include "read_chunk.hpp" //helper for reading a vector of structures from a file
#include "data_path.hpp" //helper to get paths relative to executable
#include "compile_program.hpp" //helper to compile opengl shader programs
#include "draw_text.hpp" //helper to... um.. draw text
#include "load_save_png.hpp"
#include "scene_program.hpp"
#include "depth_program.hpp"
#include "mrt_blur_program.hpp"
#include "surface_program.hpp"
#include "stylize_program.hpp"
#include "http-tweak/tweak.hpp"
#include "parameters.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>
#include <map>
#include <cstddef>
#include <random>
#include <png.h>
#include <algorithm>

#ifndef TWEAK_ENABLE
#error "http-tweak not enabled"
#endif

std::string file = "test";
Load< MeshBuffer > meshes(LoadTagDefault, [](){
	return new MeshBuffer(data_path(file+".pgct"));
});
Load< GLuint > meshes_for_scene_program(LoadTagDefault, [](){
	return new GLuint(meshes->make_vao_for_program(scene_program->program));
});
Load< GLuint > meshes_for_depth_program(LoadTagDefault, [](){
	return new GLuint(meshes->make_vao_for_program(depth_program->program));
});

//used for fullscreen passes:
Load< GLuint > empty_vao(LoadTagDefault, [](){
	GLuint vao = 0;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glBindVertexArray(0);
	return new GLuint(vao);
});

Load< GLuint > copy_program(LoadTagDefault, [](){
	GLuint program = compile_program(
		//this draws a triangle that covers the entire screen:
		"#version 330\n"
		"void main() {\n"
		"	gl_Position = vec4(4 * (gl_VertexID & 1) - 1,  2 * (gl_VertexID & 2) - 1, 0.0, 1.0);\n"
		"}\n"
		,
		//NOTE on reading screen texture:
		//texelFetch() gives direct pixel access with integer coordinates, but accessing out-of-bounds pixel is undefined:
		//	vec4 color = texelFetch(tex, ivec2(gl_FragCoord.xy), 0);
		//texture() requires using [0,1] coordinates, but handles out-of-bounds more gracefully (using wrap settings of underlying texture):
		//	vec4 color = texture(tex, gl_FragCoord.xy / textureSize(tex,0));

		"#version 330\n"
		"uniform sampler2D tex;\n"
		"out vec4 fragColor;\n"
		"void main() {\n"
		"	fragColor = texelFetch(tex, ivec2(gl_FragCoord.xy), 0);\n"
		"}\n"
	);

	glUseProgram(program);

	glUniform1i(glGetUniformLocation(program, "tex"), 0);

	glUseProgram(0);

	return new GLuint(program);
});


GLuint load_texture(std::string const &filename) {
	glm::uvec2 size;
	std::vector< glm::u8vec4 > data;
	load_png(filename, &size, &data, LowerLeftOrigin);

	GLuint tex = 0;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
	GL_ERRORS();

	return tex;
}

//texture for the platform in the test scene
Load< GLuint > grid_tex(LoadTagDefault, [](){
	return new GLuint(load_texture(data_path("textures/grid.png")));
});

//watercolor paper texture
Load< GLuint > paper_tex(LoadTagDefault, [](){
	return new GLuint(load_texture(data_path("textures/paper.png")));
});

Load< GLuint > white_tex(LoadTagDefault, [](){
	GLuint tex = 0;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glm::u8vec4 white(0xff, 0xff, 0xff, 0xff);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, glm::value_ptr(white));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	return new GLuint(tex);
});

Scene::Transform *camera_parent_transform = nullptr;
Scene::Camera *camera = nullptr;

//Options for what to show for debugging using http-tweak
enum Stages{
    VERTEX_COLORS=0,
    CONTROL_COLORS=1,
    HAND_TREMORS=2,
    PIGMENT=3,
    GAUSSIAN_BLUR=4,
    BILATERAL_BLUR=5,
    SURFACE=6,
    FINAL=7
};

//Other globals
bool surfaced = false; //so surface shader is only called once and when resizing
bool pic_mode = false;
int width, height;
GLuint screen_tex;

//Gaussian weights
float w1[1] = {1.f};
float w2[2] = {0.44198f, 0.27901f};
float w3[3] = {0.250301f, 0.221461, 0.153388f};
float w4[4] = {0.214607f, 0.189879f, 0.131514f, 0.071303f};
float w5[5] = {0.20236f, 0.179044f, 0.124009f, 0.067234f, 0.028532f};
float w6[6] = {0.141836f, 0.13424f, 0.113806f, 0.086425f, 0.05879f, 0.035822f};
float w7[7] = {0.136498f, 0.129188f, 0.109523f, 0.083173f, 0.056577f, 0.034474f, 0.018816f};
float w8[8] = {0.105915f, 0.102673f, 0.093531f, 0.080066f, 0.064408f, 0.048689f, 0.034587f, 0.023089f};
float w9[9] = {0.102934f, 0.099783f, 0.090898f, 0.077812f, 0.062595f, 0.047318f, 0.033613f, 0.022439f, 0.014076f};
float w10[10] = {0.101253f, 0.098154f, 0.089414f, 0.076542f, 0.061573f, 0.046546f, 0.033065f, 0.022072f, 0.013846f, 0.008162f};
//TODO normalize
float w11[11] = {0.082607f, 0.080977f, 0.076276f, 0.069041f, 0.060049f, 0.050187f, 0.040306f, 0.031105f, 0.023066f, 0.016436f, 0.011254f};
float w12[12] = {0.081402f, 0.079795f, 0.075163f, 0.068033f, 0.059173f, 0.049455f, 0.039717f, 0.030651f, 0.022729f, 0.016196f, 0.01109f, 0.007297f};
float w13[13] = {0.080657f, 0.079066f, 0.074476f, 0.067411f, 0.058632f, 0.049003f, 0.039354f, 0.03037f, 0.022521f, 0.016048f, 0.010989f, 0.00723f, 0.004571f};
float w14[14] = {0.068078f, 0.067141f, 0.064407f, 0.060096f, 0.054541f, 0.048146f, 0.041339f, 0.034525f, 0.028045f, 0.022159f, 0.01703f, 0.01273f, 0.009256f, 0.006546f};
float w15[15] = {0.06747f, 0.066542f, 0.063832f, 0.05956f, 0.054054f, 0.047716f, 0.04097f, 0.034216f, 0.027795f, 0.021961f, 0.016878f, 0.012617f, 0.009173f, 0.006488f, 0.004463f};
float w16[16] = {0.06707f, 0.066147f, 0.063453f, 0.059206f, 0.053733f, 0.047433f, 0.040727f, 0.034013f, 0.02763f, 0.021831f, 0.016778f, 0.012542f, 0.009119f, 0.006449f, 0.004436f, 0.002968f};
float w17[17] = {0.058012f, 0.057424f, 0.055695f, 0.05293f, 0.049287f, 0.044969f, 0.040202f, 0.035216f, 0.030226f, 0.02542f, 0.020946f, 0.016912f, 0.01338f, 0.010372f, 0.007878f, 0.005863f, 0.004275f};
float w18[18] = {0.051308f, 0.050909f, 0.049732f, 0.047829f, 0.045287f, 0.042216f, 0.038744f, 0.035007f, 0.03114f, 0.027272f, 0.023514f, 0.019961f, 0.016682f, 0.013726f, 0.011118f, 0.008867f, 0.006962f, 0.005382f};
float w19[19] = {0.046142f, 0.045858f, 0.045018f, 0.043651f, 0.041807f, 0.03955f, 0.036956f, 0.034109f, 0.031095f, 0.028001f, 0.024905f, 0.02188f, 0.018987f, 0.016274f, 0.013778f, 0.011522f, 0.009517f, 0.007765f, 0.006257f};
float w20[20] = {0.042028f, 0.041819f, 0.041197f, 0.040181f, 0.0388f, 0.037094f, 0.03511f, 0.032903f, 0.030527f, 0.028041f, 0.025502f, 0.022962f, 0.02047f, 0.018066f, 0.015787f, 0.013657f, 0.011698f, 0.00992f, 0.008329f, 0.006923f};
float* weight_arrays[] = {w1, w2, w3, w4, w5, w6, w7, w8, w9, w10, w11, w12, w13, w14, w15, w16, w17, w18, w19,w20};

//Initial scene loading setup stuff
Load< Scene > scene(LoadTagDefault, [](){
	Scene *ret = new Scene;
	//pre-build some program info (material) blocks to assign to each object:
	Scene::Object::ProgramInfo scene_program_info;
	scene_program_info.program = scene_program->program;
	scene_program_info.vao = *meshes_for_scene_program;
	scene_program_info.mvp_mat4  = scene_program->object_to_clip_mat4;
	scene_program_info.mv_mat4x3 = scene_program->object_to_light_mat4x3;
	scene_program_info.itmv_mat3 = scene_program->normal_to_light_mat3;

	Scene::Object::ProgramInfo depth_program_info;
	depth_program_info.program = depth_program->program;
	depth_program_info.vao = *meshes_for_depth_program;
	depth_program_info.mvp_mat4  = depth_program->object_to_clip_mat4;

	//load transform hierarchy:
	ret->load(data_path(file+".scene"), [&](Scene &s, Scene::Transform *t, std::string const &m){
		Scene::Object *obj = s.new_object(t);

		obj->programs[Scene::Object::ProgramTypeDefault] = scene_program_info;
		if (t->name == "Platform") {
			obj->programs[Scene::Object::ProgramTypeDefault].textures[0] = *grid_tex;
		} else {
			obj->programs[Scene::Object::ProgramTypeDefault].textures[0] = *white_tex;
		}

		obj->programs[Scene::Object::ProgramTypeShadow] = depth_program_info;

		MeshBuffer::Mesh const &mesh = meshes->lookup(m);
		obj->programs[Scene::Object::ProgramTypeDefault].start = mesh.start;
		obj->programs[Scene::Object::ProgramTypeDefault].count = mesh.count;

		obj->programs[Scene::Object::ProgramTypeShadow].start = mesh.start;
		obj->programs[Scene::Object::ProgramTypeShadow].count = mesh.count;
	});

	//look up camera parent transform:
	for (Scene::Transform *t = ret->first_transform; t != nullptr; t = t->alloc_next) {
		if (t->name == "CameraParent") {
			if (camera_parent_transform) throw std::runtime_error("Multiple 'CameraParent' transforms in scene.");
			camera_parent_transform = t;
		}
	}
	if (!camera_parent_transform) throw std::runtime_error("No 'CameraParent' transform in scene.");

    //look up the camera:
	for (Scene::Camera *c = ret->first_camera; c != nullptr; c = c->alloc_next) {
		if (c->transform->name == "Camera") {
			if (camera) throw std::runtime_error("Multiple 'Camera' objects in scene.");
			camera = c;
		}
	}
	if (!camera) throw std::runtime_error("No 'Camera' camera in scene.");

    //setting up http-tweak for debugging use
    using namespace Parameters;
    TWEAK_CONFIG(8888, data_path("../http-tweak/tweak-ui.html"));
    static TWEAK(speed);
    static TWEAK(frequency);
    static TWEAK(tremor_amount);
    static TWEAK_HINT(dA, "float 0.0001 1.0");
    static TWEAK_HINT(cangiante_variable, "float 0.0 1.0");
    static TWEAK_HINT(dilution_variable, "float 0.0 1.0");
    static TWEAK_HINT(density_amount, "float 0.0 5.0");
    static TWEAK_HINT(show, "int 0 7");
    static TWEAK_HINT(depth_threshold, "float 0.0 0.001");
    static TWEAK_HINT(blur_amount, "int 0 10");
    //TODO hmm does tweak not support bools?
   // static TWEAK_HINT(bleed, "");
   // static TWEAK_HINT(distortion, "");
	return ret;
});

GameMode::GameMode() {
}

GameMode::~GameMode() {
}

bool GameMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
	//ignore any keys that are the result of automatic key repeat:
	if (evt.type == SDL_KEYDOWN && evt.key.repeat) {
		return false;
	}

    if(evt.type == SDL_KEYDOWN){
        glm::mat3 directions = glm::mat3_cast(camera->transform->rotation);
        if(evt.key.keysym.scancode == SDL_SCANCODE_SPACE){
            std::string filename = "renders/"+Parameters::filename+".png";
            write_png(filename.c_str());
        }else if(evt.key.keysym.scancode == SDL_SCANCODE_Q){
            glm::vec3 step = -1.0f * directions[2];
            camera->transform->position+=step;
        }else if(evt.key.keysym.scancode == SDL_SCANCODE_E){
            glm::vec3 step = 1.0f * directions[2];
            camera->transform->position+=step;
        }else if(evt.key.keysym.scancode == SDL_SCANCODE_W){
            glm::vec3 step = 0.5f * directions[1];
            camera->transform->position+=step;
        }else if(evt.key.keysym.scancode == SDL_SCANCODE_S){
            glm::vec3 step = -0.5f * directions[1];
            camera->transform->position+=step;
        }else if(evt.key.keysym.scancode == SDL_SCANCODE_A){
            glm::vec3 step = -0.5f * directions[0];
            camera->transform->position+=step;
        }else if(evt.key.keysym.scancode == SDL_SCANCODE_D){
            glm::vec3 step = 0.5f * directions[0];
            camera->transform->position+=step;
        }


    }

	if (evt.type == SDL_MOUSEMOTION) {
		if (evt.motion.state & SDL_BUTTON(SDL_BUTTON_LEFT)) {
            pitch -= 5.0f * evt.motion.yrel / float(window_size.y);
            camera_rot = glm::angleAxis(pitch, glm::vec3(1.0f, 0.0f, 0.0f));

			yaw += 5.0f * evt.motion.xrel / float(window_size.x);
            camera_rot = glm::angleAxis(yaw, glm::vec3(0.0f, 0.0f, 1.0f))*camera_rot;

			return true;
		}
	}

/*    if(evt.type == SDL_MOUSEWHEEL){
		if (evt.motion.state & SDL_BUTTON(SDL_BUTTON_WHEELUP)) {
            std::cout<<"fjdlksfjdslk"<<std::endl;
        }
    }
    */
	return false;
}

void GameMode::write_png(const char *filename){
    FILE *output = fopen(filename, "wb");
    if(!output) return;
    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) return;
    png_infop info = png_create_info_struct(png);
    if(!info) return;
    if(setjmp(png_jmpbuf(png))) return;
    png_init_io(png, output);
    png_set_IHDR(png, info, width, height, 8,
            PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE,
            PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png, info);

    glBindTexture(GL_TEXTURE_2D, screen_tex);
    std::vector<GLfloat> pixels(width*height*4, 0.0f);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, pixels.data());
    png_bytep row = (png_bytep)malloc(sizeof(png_byte)*width*4);
    for(int y = height-1; y >= 0; y--) {
        for(int x = 0; x<width; x++){
            row[x*4] = glm::clamp(int32_t(pixels[(x+y*width)*4]*255), 0, 255);
            row[x*4+1] = glm::clamp(int32_t(pixels[(x+y*width)*4+1]*255), 0, 255);
            row[x*4+2] = glm::clamp(int32_t(pixels[(x+y*width)*4+2]*255), 0, 255);
            row[x*4+3] = glm::clamp(int32_t(pixels[(x+y*width)*4+3]*255), 0, 255);
        }
        png_write_row(png, row);
    }
    png_write_end(png, NULL);

    free(row);
    fclose(output);
    std::cout<<"done writing out to "<<filename<<std::endl;
}

void GameMode::update(float elapsed) {
	camera_parent_transform->rotation = glm::normalize(camera_rot);
        //glm::angleAxis(camera_spin, glm::vec3(0.0f, 0.0f, 1.0f));
    Parameters::elapsed_time+=elapsed;
    TWEAK_SYNC();
}

//GameMode will render to some offscreen framebuffer(s).
//This code allocates and resizes them as needed:
struct Textures {
	glm::uvec2 size = glm::uvec2(0,0); //remember the size of the framebuffer

    GLuint control_tex = 0;
	GLuint color_tex = 0;
	GLuint depth_tex = 0;
    GLuint blurred_tex = 0;
    GLuint bleeded_tex = 0;
    GLuint surface_tex = 0;
    GLuint final_tex = 0;
    GLuint blur_temp_tex = 0; //temp textures are for two-pass blur
    GLuint bleed_temp_tex = 0;
    GLuint control_temp_tex = 0;
    GLuint final_control_tex = 0;
	void allocate(glm::uvec2 const &new_size) {
    //allocate full-screen framebuffer:
		if (size != new_size) {
            std::cout<<new_size.x<<" "<<new_size.y<<std::endl;
			size = new_size;
            width = size.x;
            height = size.y;
            surfaced = false;
            auto alloc_tex = [this](GLuint *tex, GLint internalformat, GLint format){
                if (*tex == 0) glGenTextures(1, tex);
	    		glBindTexture(GL_TEXTURE_2D, *tex);
		    	glTexImage2D(GL_TEXTURE_2D, 0, internalformat, size.x,
                        size.y, 0, format, GL_UNSIGNED_BYTE, NULL);
			    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	    		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		    	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			    glBindTexture(GL_TEXTURE_2D, 0);
            };

            alloc_tex(&control_tex, GL_RGBA32F, GL_RGBA);
            alloc_tex(&color_tex, GL_RGBA8, GL_RGBA);
            alloc_tex(&depth_tex, GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT);
            alloc_tex(&blurred_tex, GL_RGBA32F, GL_RGBA);
            alloc_tex(&blur_temp_tex, GL_RGBA32F, GL_RGBA);
            alloc_tex(&bleed_temp_tex, GL_RGBA32F, GL_RGBA);
            alloc_tex(&control_temp_tex, GL_RGBA32F, GL_RGBA);
            alloc_tex(&final_control_tex, GL_RGBA32F, GL_RGBA);
            alloc_tex(&bleeded_tex, GL_RGBA32F, GL_RGBA);
            alloc_tex(&surface_tex, GL_RGBA8, GL_RGBA);
            alloc_tex(&final_tex, GL_RGBA8, GL_RGBA);
			GL_ERRORS();
		}

	}
} textures;

//renders the color texture, control texture, and depth buffer.
void GameMode::draw_scene(GLuint* color_tex_, GLuint* control_tex_,
                        GLuint* depth_tex_){
    assert(color_tex_);
    assert(control_tex_);
    assert(depth_tex_);
    auto &color_tex = *color_tex_;
    auto &control_tex = *control_tex_;
    auto &depth_tex = *depth_tex_;

    /* backing up the variables in case they need to be briefly turned off,
     * like when viewing only color texture or control texture, so speed should
     * be 0 in order to avoid seeing the handtremors.
     */
    float dA0 =  Parameters::dA;
    float cangiante_variable0 = Parameters::cangiante_variable;
    float dilution_variable0 =  Parameters::dilution_variable;
    float speed0 =  Parameters::speed;
    if(Parameters::show < PIGMENT){
        Parameters::dA = 0.f;
        Parameters::cangiante_variable = 0.f;
        Parameters::dilution_variable = 0.f;
        if(Parameters::show<HAND_TREMORS)
            Parameters::speed = 0.f;
    }
    //Textures to draw into
    static GLuint fb = 0;
    if(fb==0) glGenFramebuffers(1, &fb);
    glBindFramebuffer(GL_FRAMEBUFFER, fb);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                            color_tex, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D,
                            control_tex, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                            depth_tex, 0);
    GLenum bufs[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, bufs);
    check_fb();


	//Draw scene to off-screen framebuffer:
	glBindFramebuffer(GL_FRAMEBUFFER, fb);
	glViewport(0,0, textures.size.x, textures.size.y);
	camera->aspect = textures.size.x / float(textures.size.y);

    GLfloat white[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    GLfloat black[4] = {0.0f, 0.5f, 0.0f, 0.0f};
    glClearBufferfv(GL_COLOR, 0, white);
    glClearBufferfv(GL_COLOR, 1, black);
	glClear(GL_DEPTH_BUFFER_BIT);

	//set up basic OpenGL state:
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glUseProgram(scene_program->program);

	glUniform3fv(scene_program->sun_color_vec3, 1, glm::value_ptr(glm::vec3(0.5f, 0.5f, 0.5f)));
	glUniform3fv(scene_program->sun_direction_vec3, 1, glm::value_ptr(glm::normalize(glm::vec3(0.5f, 0.3f, 1.f))));
	glUniform3fv(scene_program->sky_color_vec3, 1, glm::value_ptr(glm::vec3(0.2f, 0.2f, 0.2f)));
	glUniform3fv(scene_program->sky_direction_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f, 1.0f)));
    glUniform1f(scene_program->time, Parameters::elapsed_time);
    glUniform1f(scene_program->speed,  Parameters::speed);
    glUniform1f(scene_program->frequency,  Parameters::frequency);
    glUniform1f(scene_program->tremor_amount,  Parameters::tremor_amount);
    //view coords go from -1 to 1, so thats 2.0/# of pixels
    glUniform2fv(scene_program->clip_units_per_pixel, 1,
        glm::value_ptr(glm::vec2(2.f/textures.size.x, 2.f/textures.size.y)));
    glUniform3fv(scene_program->viewPos, 1,
            glm::value_ptr(camera->transform->make_local_to_world()));
    glUniform1f(scene_program->dA, Parameters::dA);
    glUniform1f(scene_program->cangiante_variable, Parameters::cangiante_variable);
    glUniform1f(scene_program->dilution_variable,Parameters::dilution_variable);
    scene->draw(camera);
    //restoring things turned off for debug view
    Parameters::dA = dA0;
    Parameters::cangiante_variable = cangiante_variable0;
    Parameters::dilution_variable = dilution_variable0;
    Parameters::speed = speed0;
}

//copies weights into the array that'll be passed as an uniform
void GameMode::get_weights(){
    if(Parameters::blur_amount>0 && Parameters::blur_amount<=20){
        auto to_copy = weight_arrays[Parameters::blur_amount-1];
        for(int i = 0; i<Parameters::blur_amount; i++){
            weights[i] = to_copy[i];
        }
    }
}

//shader that does a horizontal pass and a vertical pass of gaussian blur and
//bilateral blur
void GameMode::draw_mrt_blur(GLuint color_tex, GLuint control_tex,
        GLuint depth_tex, GLuint *blur_temp_tex_, GLuint *bleed_temp_tex_,
        GLuint *control_temp_tex_, GLuint* final_control_tex_,
        GLuint* blurred_tex_, GLuint* bleeded_tex_){
    assert(blurred_tex_);
    assert(bleeded_tex_);
    assert(blur_temp_tex_);
    assert(bleed_temp_tex_);
    assert(control_temp_tex_);
    assert(final_control_tex_);
    auto &blur_temp_tex = *blur_temp_tex_;
    auto &bleed_temp_tex = *bleed_temp_tex_;
    auto &control_temp_tex = *control_temp_tex_;
    auto &final_control_tex = *final_control_tex_;
    auto &blurred_tex = *blurred_tex_;
    auto &bleeded_tex = *bleeded_tex_;

    /*since shaders can't write into the textures they read from, the
     * horizontal pass draws into the temp versions of the buffers. The
     * vertical pass then reads the temp versions and draws into the non-temp*/
    static GLuint fb = 0;
    if(fb==0) glGenFramebuffers(1, &fb);
    glBindFramebuffer(GL_FRAMEBUFFER, fb);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                            blur_temp_tex, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D,
                            bleed_temp_tex, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D,
                            control_temp_tex, 0);
    {
        GLenum bufs[3] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
        glDrawBuffers(3, bufs);
    }
    check_fb();

    //set glViewport
	glBindFramebuffer(GL_FRAMEBUFFER, fb);
	glViewport(0,0, textures.size.x, textures.size.y);
	camera->aspect = textures.size.x / float(textures.size.y);

    GLfloat black[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    glClearBufferfv(GL_COLOR, 0, black);
    glClearBufferfv(GL_COLOR, 1, black);

	//set up basic OpenGL state:
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

    //read from unblurred color_tex for both gaussian and bilateral blur
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, color_tex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, color_tex);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, control_tex);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, depth_tex);

    glUseProgram(mrt_blurH_program->program);
    glUniform1f(mrt_blurH_program->depth_threshold, Parameters::depth_threshold);
    glUniform1i(mrt_blurH_program->blur_amount, Parameters::blur_amount);
    get_weights();
    glUniform1fv(mrt_blurH_program->weights, 20, weights);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    static GLuint fb2 = 0;
    if(fb2==0) glGenFramebuffers(1, &fb2);
    glBindFramebuffer(GL_FRAMEBUFFER, fb2);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                            blurred_tex, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D,
                            bleeded_tex, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D,
                            final_control_tex, 0);
    {
        GLenum bufs[3] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
        glDrawBuffers(3, bufs);
    }
    check_fb();

    //read from horizontally blurred blur and bleed textures to do vertical pass
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, blur_temp_tex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, bleed_temp_tex);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, control_temp_tex);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, depth_tex);

    glUseProgram(mrt_blurV_program->program);
    glUniform1f(mrt_blurV_program->depth_threshold, Parameters::depth_threshold);
    glUniform1i(mrt_blurV_program->blur_amount,Parameters:: blur_amount);
    glUniform1fv(mrt_blurV_program->weights, 20, weights);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, 0);

}

/* draws the surface shader, which contains a heightmap, normal map,
 * and rendered paper
 */
void GameMode::draw_surface(GLuint paper_tex, GLuint* surface_tex_){
    surfaced = true;
    assert(surface_tex_);
    auto &surface_tex = *surface_tex_;

    static GLuint fb = 0;
    if(fb==0) glGenFramebuffers(1, &fb);
    glBindFramebuffer(GL_FRAMEBUFFER, fb);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                            surface_tex, 0);
    GLenum bufs[1] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, bufs);
    check_fb();

    //set glViewport
	glBindFramebuffer(GL_FRAMEBUFFER, fb);
	glViewport(0,0, textures.size.x, textures.size.y);
	camera->aspect = textures.size.x / float(textures.size.y);

    GLfloat black[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    glClearBufferfv(GL_COLOR, 0, black);

	//set up basic OpenGL state:
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, paper_tex);

	glUseProgram(surface_program->program);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

//combines paper effects, edge darkening, and bleeding into final texture
void GameMode::draw_stylization(GLuint color_tex, GLuint final_control_tex,
                            GLuint surface_tex, GLuint blurred_tex,
                            GLuint bleeded_tex, GLuint* final_tex_){
    assert(final_tex_);
    auto &final_tex = *final_tex_;

    static GLuint fb = 0;
    if(fb==0) glGenFramebuffers(1, &fb);
    glBindFramebuffer(GL_FRAMEBUFFER, fb);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                            final_tex, 0);

    GLenum bufs[1] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, bufs);
    check_fb();

    //set glViewport
	glBindFramebuffer(GL_FRAMEBUFFER, fb);
	glViewport(0,0, textures.size.x, textures.size.y);
	camera->aspect = textures.size.x / float(textures.size.y);

    GLfloat black[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    glClearBufferfv(GL_COLOR, 0, black);

	//set up basic OpenGL state:
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, color_tex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, final_control_tex);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, blurred_tex);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, bleeded_tex);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, surface_tex);

	glUseProgram(stylize_program->program);
    glUniform1f(stylize_program->density_amount, Parameters::density_amount);
    glUniform1f(stylize_program->bleed, Parameters::bleed);
    glUniform1f(stylize_program->distortion, Parameters::distortion);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, 0);
}

//main draw function that calls the functions that call the other shaders
void GameMode::draw(glm::uvec2 const &drawable_size) {
	textures.allocate(drawable_size);

    draw_scene(&textures.color_tex, &textures.control_tex, &textures.depth_tex);
    draw_mrt_blur(textures.color_tex, textures.control_tex, textures.depth_tex,
                &textures.blur_temp_tex, &textures.bleed_temp_tex,
                &textures.control_temp_tex, &textures.final_control_tex,
                &textures.blurred_tex, &textures.bleeded_tex);

    //only needs to be updated when resized since it doesn't change
    if(!surfaced)
        draw_surface(*paper_tex, &textures.surface_tex);
    draw_stylization(textures.color_tex, textures.final_control_tex,
            textures.surface_tex, textures.blurred_tex, textures.bleeded_tex,
            &textures.final_tex);

	//Copy scene from color buffer to screen, performing post-processing effects:
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glActiveTexture(GL_TEXTURE0);
    if(Parameters::show == FINAL){ //show different parts of pipeline for debug use
    	glBindTexture(GL_TEXTURE_2D, textures.final_tex);
        screen_tex = textures.final_tex;
    }else if(Parameters::show == CONTROL_COLORS){
        glBindTexture(GL_TEXTURE_2D, textures.control_tex);
        screen_tex = textures.control_tex;
    }else if(Parameters::show == GAUSSIAN_BLUR){
        glBindTexture(GL_TEXTURE_2D, textures.blurred_tex);
        screen_tex = textures.blurred_tex;
    }else if(Parameters::show == BILATERAL_BLUR){
        glBindTexture(GL_TEXTURE_2D, textures.bleeded_tex);
        screen_tex = textures.bleeded_tex;
    }else if(Parameters::show == SURFACE){
        glBindTexture(GL_TEXTURE_2D, textures.surface_tex);
        screen_tex = textures.surface_tex;
    }else{
        glBindTexture(GL_TEXTURE_2D, textures.color_tex);
        screen_tex = textures.color_tex;
    }

    if(pic_mode){
        std::string filename = "renders/"+Parameters::filename+".png";
        write_png(filename.c_str());
        Mode::set_current(nullptr);
    }

    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
	glUseProgram(*copy_program);
	glBindVertexArray(*empty_vao);

	glDrawArrays(GL_TRIANGLES, 0, 3);

	glUseProgram(0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
    GL_ERRORS();
}

