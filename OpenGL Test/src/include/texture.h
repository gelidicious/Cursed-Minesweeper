#ifndef TEXTURE_H
#define TEXTURE_H

#include <glad/glad.h>
#include <stb/stb_image.h>

#include <iostream>

class Texture {
private:
	GLuint id;
	GLenum textureType;
	GLuint textureUnit;
public:
	Texture(const char* path, GLenum textureType = GL_TEXTURE_2D, GLenum format = GL_RGBA, GLenum pixelType = GL_UNSIGNED_BYTE);
	~Texture();
	void unit(GLuint program, const char* uniform, GLuint unit);
	void unitBind();
	void bind();
	void unbind();
	void free();
};

#endif