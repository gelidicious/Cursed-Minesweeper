#include "include/texture.h"

Texture::Texture(const char* path, GLenum textureType, GLenum format, GLenum pixelType) {
    this->textureType = textureType;

    glGenTextures(1, &id);
    glBindTexture(textureType, id);

    glTexParameteri(textureType, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(textureType, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    int width, height, colorChannels;
    unsigned char* data = stbi_load(path, &width, &height, &colorChannels, 0);

    if (!data) {
        std::cout << "failed to load image, reason: " << stbi_failure_reason();
    }

    glTexImage2D(textureType, 0, format, width, height, 0, format, pixelType, data);
    glGenerateMipmap(textureType);

    stbi_image_free(data);
}

Texture::~Texture() {}

void Texture::unit(GLuint program, const char* uniform, GLuint unit) {
    textureUnit = unit;

    glUseProgram(program);

    glUniform1i(glGetUniformLocation(program, uniform), static_cast<GLint>(unit));

    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(textureType, id);
}

void Texture::unitBind() {
    glActiveTexture(GL_TEXTURE0 + textureUnit);
    glBindTexture(textureType, id);
}

void Texture::bind() {
    glBindTexture(textureType, id);
}

void Texture::unbind() {
    glBindTexture(textureType, 0);
}

void Texture::free() {
	glDeleteTextures(1, &id);
}