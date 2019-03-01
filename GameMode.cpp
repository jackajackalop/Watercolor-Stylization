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

#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>
#include <map>
#include <cstddef>
#include <random>

#ifndef TWEAK_ENABLE
#error "poop"
#endif
Load< MeshBuffer > meshes(LoadTagDefault, [](){
	return new MeshBuffer(data_path("test.pgct"));
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

Load< GLuint > wood_tex(LoadTagDefault, [](){
	return new GLuint(load_texture(data_path("textures/grid.png")));
});

Load< GLuint > marble_tex(LoadTagDefault, [](){
	return new GLuint(load_texture(data_path("textures/marble.png")));
});

Load< GLuint > paper_tex(LoadTagDefault, [](){
	return new GLuint(load_texture(data_path("textures/marble.png")));
});

Load< GLuint > normal_map_tex(LoadTagDefault, [](){
	return new GLuint(load_texture(data_path("textures/wood.png")));
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
//Scene::Transform *spot_parent_transform = nullptr;
//Scene::Lamp *spot = nullptr;

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

float elapsed_time = 0.0f;
float speed = 5.f;
float frequency = 0.2f;
float tremor_amount = 1.f;
float dA = 0.2f;
float cangiante_variable = 0.1f;
float dilution_variable = 0.35f;
//int show = GAUSSIAN_BLUR;
int show = BILATERAL_BLUR;
float depth_threshold = 0.f;
int blur_amount = 5;

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

float* weight_arrays[] = {w1, w2, w3, w4, w5, w6, w7, w8, w9, w10};

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
	ret->load(data_path("test.scene"), [&](Scene &s, Scene::Transform *t, std::string const &m){
		Scene::Object *obj = s.new_object(t);

		obj->programs[Scene::Object::ProgramTypeDefault] = scene_program_info;
		if (t->name == "Platform") {
			obj->programs[Scene::Object::ProgramTypeDefault].textures[0] = *wood_tex;
		} else if (t->name == "Pedestal") {
			obj->programs[Scene::Object::ProgramTypeDefault].textures[0] = *marble_tex;
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
/*		if (t->name == "SpotParent") {
			if (spot_parent_transform) throw std::runtime_error("Multiple 'SpotParent' transforms in scene.");
			spot_parent_transform = t;
		}
*/
	}
	if (!camera_parent_transform) throw std::runtime_error("No 'CameraParent' transform in scene.");
//	if (!spot_parent_transform) throw std::runtime_error("No 'SpotParent' transform in scene.");

	//look up the camera:
	for (Scene::Camera *c = ret->first_camera; c != nullptr; c = c->alloc_next) {
		if (c->transform->name == "Camera") {
			if (camera) throw std::runtime_error("Multiple 'Camera' objects in scene.");
			camera = c;
		}
	}
	if (!camera) throw std::runtime_error("No 'Camera' camera in scene.");
/*
	//look up the spotlight:
	for (Scene::Lamp *l = ret->first_lamp; l != nullptr; l = l->alloc_next) {
		if (l->transform->name == "Spot") {
			if (spot) throw std::runtime_error("Multiple 'Spot' objects in scene.");
			if (l->type != Scene::Lamp::Spot) throw std::runtime_error("Lamp 'Spot' is not a spotlight.");
			spot = l;
		}
	}
	if (!spot) throw std::runtime_error("No 'Spot' spotlight in scene.");
*/
    TWEAK_CONFIG(8888, data_path("../http-tweak/tweak-ui.html"));
    static TWEAK(speed);
    static TWEAK(frequency);
    static TWEAK(tremor_amount);
    static TWEAK_HINT(dA, "float 0.0001 1.0");
    static TWEAK_HINT(cangiante_variable, "float 0.0 1.0");
    static TWEAK_HINT(dilution_variable, "float 0.0 1.0");
    static TWEAK_HINT(show, "int 0 7");
    static TWEAK_HINT(depth_threshold, "float 0.0 1.0");
    static TWEAK_HINT(blur_amount, "int 0 10");
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

	if (evt.type == SDL_MOUSEMOTION) {
		if (evt.motion.state & SDL_BUTTON(SDL_BUTTON_LEFT)) {
			camera_spin += 5.0f * evt.motion.xrel / float(window_size.x);
			return true;
		}
		if (evt.motion.state & SDL_BUTTON(SDL_BUTTON_RIGHT)) {
			//spot_spin += 5.0f * evt.motion.xrel / float(window_size.x);
			return true;
		}

	}

	return false;
}

void GameMode::update(float elapsed) {
	camera_parent_transform->rotation = glm::angleAxis(camera_spin, glm::vec3(0.0f, 0.0f, 1.0f));
//	spot_parent_transform->rotation = glm::angleAxis(spot_spin, glm::vec3(0.0f, 0.0f, 1.0f));
    elapsed_time+=elapsed;
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
    GLuint blur_temp_tex = 0;
    GLuint bleed_temp_tex = 0;
    GLuint control_temp_tex = 0;
	void allocate(glm::uvec2 const &new_size) {
    //allocate full-screen framebuffer:

		if (size != new_size) {
			size = new_size;

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
            alloc_tex(&bleeded_tex, GL_RGBA32F, GL_RGBA);
            alloc_tex(&surface_tex, GL_RGBA8, GL_RGBA);
            alloc_tex(&final_tex, GL_RGBA8, GL_RGBA);
			GL_ERRORS();
		}

	}
} textures;

void GameMode::draw_scene(GLuint* color_tex_, GLuint* control_tex_,
                        GLuint* depth_tex_){
    assert(color_tex_);
    assert(control_tex_);
    assert(depth_tex_);
    auto &color_tex = *color_tex_;
    auto &control_tex = *control_tex_;
    auto &depth_tex = *depth_tex_;

    float dA0 = dA;
    float cangiante_variable0 = cangiante_variable;
    float dilution_variable0 = dilution_variable;
    float speed0 = speed;
    if(show < PIGMENT){
        dA = 0.f;
        cangiante_variable = 0.f;
        dilution_variable = 0.f;
        if(show<HAND_TREMORS)
            speed = 0.f;
    }
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

    GLfloat black[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    glClearBufferfv(GL_COLOR, 0, black);
    glClearBufferfv(GL_COLOR, 1, black);
	glClear(GL_DEPTH_BUFFER_BIT);

	//set up basic OpenGL state:
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glUseProgram(scene_program->program);

	//don't use distant directional light at all (color == 0):
	glUniform3fv(scene_program->sun_color_vec3, 1, glm::value_ptr(glm::vec3(0.5f, 0.5f, 0.5f)));
	glUniform3fv(scene_program->sun_direction_vec3, 1, glm::value_ptr(glm::normalize(glm::vec3(0.5f, 0.1f, 1.f))));
	//use hemisphere light for subtle ambient light:
	glUniform3fv(scene_program->sky_color_vec3, 1, glm::value_ptr(glm::vec3(0.f, 0.f, 0.f)));
	glUniform3fv(scene_program->sky_direction_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f, 1.0f)));
    glUniform1f(scene_program->time, elapsed_time);
    glUniform1f(scene_program->speed, speed);
    glUniform1f(scene_program->frequency, frequency);
    glUniform1f(scene_program->tremor_amount, tremor_amount);
    glUniform2fv(scene_program->clip_units_per_pixel, 1,
        glm::value_ptr(glm::vec2(2.f/textures.size.x, 2.f/textures.size.y)));
    glUniform3fv(scene_program->viewPos, 1,
            glm::value_ptr(camera->transform->make_local_to_world()));
    glUniform1f(scene_program->dA, dA);
    glUniform1f(scene_program->cangiante_variable, cangiante_variable);
    glUniform1f(scene_program->dilution_variable, dilution_variable);
    scene->draw(camera);
    dA = dA0;
    cangiante_variable = cangiante_variable0;
    dilution_variable = dilution_variable0;
    speed = speed0;
}

void GameMode::get_weights(){
    if(blur_amount>0 && blur_amount<=10){
        auto to_copy = weight_arrays[blur_amount-1];
        for(int i = 0; i<blur_amount; i++){
            weights[i] = to_copy[i];
        }
    }
}

void GameMode::draw_mrt_blur(GLuint color_tex, GLuint control_tex,
        GLuint depth_tex, GLuint *blur_temp_tex_, GLuint *bleed_temp_tex_,
        GLuint *control_temp_tex_, GLuint* blurred_tex_, GLuint* bleeded_tex_){
    assert(blurred_tex_);
    assert(bleeded_tex_);
    assert(blur_temp_tex_);
    assert(bleed_temp_tex_);
    assert(control_temp_tex_);
    auto &blur_temp_tex = *blur_temp_tex_;
    auto &bleed_temp_tex = *bleed_temp_tex_;
    auto &control_temp_tex = *control_temp_tex_;
    auto &blurred_tex = *blurred_tex_;
    auto &bleeded_tex = *bleeded_tex_;

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

    GLfloat black[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    glClearBufferfv(GL_COLOR, 0, black);
    glClearBufferfv(GL_COLOR, 1, black);

	//set up basic OpenGL state:
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, color_tex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, color_tex);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, control_tex);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, depth_tex);

    glUseProgram(mrt_blurH_program->program);
    glUniform1f(mrt_blurH_program->depth_threshold, depth_threshold);
    glUniform1i(mrt_blurH_program->blur_amount, blur_amount);
    get_weights();
    glUniform1fv(mrt_blurH_program->weights, 20, weights);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    //blurred_tex = blur_temp_tex;
    //bleeded_tex = bleed_temp_tex;
    //control_tex = control_temp_tex;
    static GLuint fb2 = 0;
    if(fb2==0) glGenFramebuffers(1, &fb2);
    glBindFramebuffer(GL_FRAMEBUFFER, fb2);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                            blurred_tex, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D,
                            bleeded_tex, 0);
    {
        GLenum bufs[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
        glDrawBuffers(2, bufs);
    }
    check_fb();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, blur_temp_tex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, bleed_temp_tex);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, control_temp_tex);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, depth_tex);

    glUseProgram(mrt_blurV_program->program);
    glUniform1f(mrt_blurV_program->depth_threshold, depth_threshold);
    glUniform1i(mrt_blurV_program->blur_amount, blur_amount);
    glUniform1fv(mrt_blurV_program->weights, 20, weights);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    //TODO unbind textues

}

void GameMode::draw_surface(GLuint paper_tex, GLuint normal_map_tex,
                            GLuint* surface_tex_){
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
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, paper_tex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, normal_map_tex);

	glUseProgram(surface_program->program);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

void GameMode::draw_stylization(GLuint color_tex, GLuint control_tex,
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
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, color_tex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, control_tex);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, blurred_tex);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, bleeded_tex);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, surface_tex);

	glUseProgram(stylize_program->program);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

void GameMode::draw(glm::uvec2 const &drawable_size) {
	textures.allocate(drawable_size);

    draw_scene(&textures.color_tex, &textures.control_tex, &textures.depth_tex);
    draw_mrt_blur(textures.color_tex, textures.control_tex, textures.depth_tex,
                &textures.blur_temp_tex, &textures.bleed_temp_tex,
                &textures.control_temp_tex, &textures.blurred_tex,
                &textures.bleeded_tex);
    draw_surface(*paper_tex, *normal_map_tex, &textures.surface_tex);
    draw_stylization(textures.color_tex, textures.control_tex,
            textures.surface_tex, textures.blurred_tex, textures.bleeded_tex,
            &textures.final_tex);
    glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	GL_ERRORS();

	//Copy scene from color buffer to screen, performing post-processing effects:
	glActiveTexture(GL_TEXTURE0);
    if(show == FINAL)
    	glBindTexture(GL_TEXTURE_2D, textures.final_tex);
    else if(show == CONTROL_COLORS)
        glBindTexture(GL_TEXTURE_2D, textures.control_tex);
    else if(show == GAUSSIAN_BLUR)
        glBindTexture(GL_TEXTURE_2D, textures.blurred_tex);
    else if(show == BILATERAL_BLUR)
        glBindTexture(GL_TEXTURE_2D, textures.bleeded_tex);
    else if(show == SURFACE)
        glBindTexture(GL_TEXTURE_2D, textures.surface_tex);
    else
        glBindTexture(GL_TEXTURE_2D, textures.color_tex);

    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
	glUseProgram(*copy_program);
	glBindVertexArray(*empty_vao);

	glDrawArrays(GL_TRIANGLES, 0, 3);

	glUseProgram(0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
}
