#pragma once

/*
 * With the introduction of Mode, there is a need to synchronize resource loading
 * among different compilation units.
 *
 * A Load< T > does this, by allowing you to write:
 *
 * //at global scope:
 * Load< Mesh > main_mesh([]() -> Mesh const * {
 *     return &Meshes.get("Main");
 * });
 *
 * //later:
 * void GameMode::draw() {
 *     glBindVertexArray(main_mesh->vao);
 * }
 *
 * Load<> is built on the add_load_function() call that adds a function to one of several lists of functions that are called after the OpenGL canvas is initialized.
 *
 * These functions are grouped by 'tags', which allow some sequencing of calls.
 * (particularly, this is useful for loading large data blobs [e.g. "Meshes"] before looking up individual elements within them.)
 *
 */

#include <functional>
#include <stdexcept>

enum LoadTag : uint32_t {
	LoadTagInit = 0, //used for loading mesh and texture blobs before main
	LoadTagDefault = 1,
	LoadTagLate = 2,
	LoadTagCount = 3
};

void add_load_function(LoadTag tag, std::function< void() > const &fn);
void call_load_functions(); //called by main() after GL context created.

template< typename T >
struct Load {
	//Constructing a Load< T > adds the passed function to the list of functions to call:
	Load( LoadTag tag, const std::function< T const *() > &load_fn ) : value(nullptr) {
		add_load_function(tag, [this,load_fn](){
			this->value = load_fn();
			if (!(this->value)) {
				throw std::runtime_error("Loading failed.");
			}
		});
	}

	//Make a "Load< T >" behave like a "T const *":
	explicit operator bool() { return value != nullptr; }
	T const &operator*() { return *value; }
	T const *operator->() { return value; }

	T const *value;
};

