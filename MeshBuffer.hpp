#pragma once

#include "GL.hpp"
#include <map>

//"MeshBuffer" holds a collection of meshes loaded from a file
// (note that meshes in a single collection will share a vbo/vao)

struct MeshBuffer {
	GLuint vbo = 0; //OpenGL vertex buffer object containing the meshes' data

	//Attrib includes location within the vertex buffer of various attributes:
	// (exactly the parameters to glVertexAttribPointer)
	struct Attrib {
		GLint size = 0;
		GLenum type = 0;
		GLboolean normalized = GL_FALSE;
		GLsizei stride = 0;
		GLsizei offset = 0;

		Attrib() = default;
		Attrib(GLint size_, GLenum type_, GLboolean normalized_, GLsizei stride_, GLsizei offset_)
		: size(size_), type(type_), normalized(normalized_), stride(stride_), offset(offset_) { }
	};

	Attrib Position;
	Attrib Normal;
	Attrib Color;
	Attrib TexCoord;


	//construct from a file:
	// note: will throw if file fails to read.
	MeshBuffer(std::string const &filename);

	//look up a particular mesh in the DB:
	// note: will throw if mesh not found.
	struct Mesh {
		GLuint start = 0;
		GLuint count = 0;
	};
	const Mesh &lookup(std::string const &name) const;
	
	//build a vertex array object that links this vbo to attributes to a program:
	//  will throw if program defines attributes not contained in this buffer
	//  and warn if this buffer contains attributes not active in the program
	GLuint make_vao_for_program(GLuint program) const;

	//internals:
	std::map< std::string, Mesh > meshes;
};
