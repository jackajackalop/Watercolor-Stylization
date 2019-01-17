#pragma once

#include "GL.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <vector>
#include <list>
#include <functional>
#include <string>

//"Scene" manages a hierarchy of transformations with, potentially, attached information.
struct Scene {

	struct Transform {
		//useful to know sometimes:
		std::string name;

		//simple specification:
		glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::quat rotation = glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
		glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f);

		//hierarchy information:
		Transform *parent = nullptr;
		Transform *last_child = nullptr;
		Transform *prev_sibling = nullptr;
		Transform *next_sibling = nullptr;
		//Generally, you shouldn't manipulate the above pointers directly.

		//Add transform to the child list of 'parent', before child 'before' (or at end, if 'before' is not given):
		void set_parent(Transform *parent, Transform *before = nullptr);

		//helper that checks local pointer consistency:
		void DEBUG_assert_valid_pointers() const;

		//computed from the above:
		glm::mat4 make_local_to_parent() const;
		glm::mat4 make_parent_to_local() const;
		glm::mat4 make_local_to_world() const;
		glm::mat4 make_world_to_local() const;

		//constructor/destructor:
		Transform() = default;
		Transform(Transform &) = delete;
		~Transform() {
			while (last_child) {
				last_child->set_parent(nullptr);
			}
			if (parent) {
				set_parent(nullptr);
			}
		}

		//used by Scene to manage allocation:
		Transform **alloc_prev_next = nullptr;
		Transform *alloc_next = nullptr;
	};

	//"Object"s contain information needed to render meshes:
	struct Object {
		Transform *transform; //objects must be attached to transforms.
		Object(Transform *transform_) : transform(transform_) {
			assert(transform);
		}

		//program info:
		enum ProgramType : uint32_t {
			ProgramTypeDefault = 0,
			ProgramTypeShadow = 1,
			ProgramTypes //count of program types
		};
		struct ProgramInfo {
			GLuint program = 0;

			//attributes:
			GLuint vao = 0;
			GLuint start = 0;
			GLuint count = 0;

			//uniforms:
			GLuint mvp_mat4 = -1U; //uniform index for object-to-clip matrix (mat4)
			GLuint mv_mat4x3 = -1U; //uniform index for model-to-lighting-space matrix (mat4x3)
			GLuint itmv_mat3 = -1U; //uniform index for normal-to-lighting-space matrix (mat3)
			std::function< void() > set_uniforms; //(optional) function to set additional uniforms

			//textures:
			enum : uint32_t { TextureCount = 4 };
			GLuint textures[TextureCount] = {0,0,0,0}; //textures to bind
		} programs[ProgramTypes];

		//used by Scene to manage allocation:
		Object **alloc_prev_next = nullptr;
		Object *alloc_next = nullptr;
	};

	//"Lamp"s contain information about lights:
	struct Lamp {
		Transform *transform; //lamps must be attached to transforms.
		Lamp(Transform *transform_) : transform(transform_) {
			assert(transform);
		}
		enum Type : char {
			Point = 'p',
			Hemisphere = 'h',
			Spot = 's',
			Directional = 'd'
		} type = Point;

		//NOTE: lights are directed along their -z axis
		glm::vec3 energy = glm::vec3(1.0f);

		float fov = glm::radians(45.0f); //fov (spot)

		//near and far planes for shadow maps:
		float clip_start = 0.1f;
		float clip_end = 100.0f;

		//computed from the above:
		glm::mat4 make_projection() const;

		//computed from the above:
		glm::mat4 make_spot_projection() const;

		//used by Scene to manage allocation:
		Lamp **alloc_prev_next = nullptr;
		Lamp *alloc_next = nullptr;
	};

	//"Camera"s contain information needed to view a scene:
	struct Camera {
		Transform *transform; //cameras must be attached to transforms.
		Camera(Transform *transform_) : transform(transform_) {
			assert(transform);
		}
		//NOTE: cameras look along their -z axis

		//camera parameters (perspective):
		float fovy = glm::radians(60.0f); //vertical fov (in radians)
		float aspect = 1.0f; //x / y
		float near = 0.01f; //near plane
		//computed from the above:
		glm::mat4 make_projection() const;

		//used by Scene to manage allocation:
		Camera **alloc_prev_next = nullptr;
		Camera *alloc_next = nullptr;
	};

	//------ functions to create / destroy scene things -----
	//NOTE: all scene objects are automatically freed when scene is deallocated

	//Create a new transform:
	Transform *new_transform();
	//Delete an existing transform: (NOTE: it is an error to delete a transform with an attached Object or Camera)
	void delete_transform(Transform *);

	//Create a new object attached to a transform:
	Object *new_object(Transform *transform);
	//Delete an object:
	void delete_object(Object *);

	//Create a new lamp attached to a transform:
	Lamp *new_lamp(Transform *transform);
	//Delete a lamp:
	void delete_lamp(Lamp *);

	//Create a new camera attached to a transform:
	Camera *new_camera(Transform *transform);
	//Delete a camera:
	void delete_camera(Camera *);

	//used to manage allocated objects:
	Transform *first_transform = nullptr;
	Object *first_object = nullptr;
	Lamp *first_lamp = nullptr;
	Camera *first_camera = nullptr;
	//(you shouldn't be manipulating these pointers directly

	//------ functions to traverse the scene ------

	//Draw the scene from a given camera by computing appropriate matrices and sending all objects to OpenGL:
	//"camera" must be non-null!
	void draw(Camera const *camera, Object::ProgramType = Object::ProgramTypeDefault ) const;

	//Draw the scene from a given lamp by computing appropriate matrices and sending all objects to OpenGL:
	//"lamp" must be non-null!
	void draw(Lamp const *lamp, Object::ProgramType = Object::ProgramTypeDefault ) const;

	//More general draw function. Will render with a specified projection transformation and use programs in the given slot of all objects:
	void draw(
		glm::mat4 const &world_to_clip,
		Object::ProgramType program_type) const;

	~Scene(); //destructor deallocates transforms, objects, cameras

	//add transforms/objects/cameras from a scene file:
	// the 'on_object' callback gives you a chance to look up a mesh by name and make an object.
	void load(std::string const &filename,
		std::function< void(Scene &, Transform *, std::string const &) > const &on_object = nullptr
	);
};
