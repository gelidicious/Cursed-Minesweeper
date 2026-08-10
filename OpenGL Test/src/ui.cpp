#include "include/ui.h"

UIElement::UIElement(float x, float y, float width, float height, float r, float g, float b, float a) {
    this->x = y;
    this->x = y;
    this->width = width;
    this->height = height;
    color[0] = r;
    color[1] = g;
    color[2] = b;
    color[3] = a;
}

UIElement::~UIElement() {}

void UIElement::addVertices(std::vector<Vertex>& vertices, std::vector<GLuint>& indices) {
    GLuint vertexCount = static_cast<GLuint>(vertices.size());

    // VBO
    vertices.push_back({ x, y, color[0], color[1], color[2], color[3], 0.0f, 0.0f });
    vertices.push_back({ x + width, y, color[0], color[1], color[2], color[3], 1.0f, 0.0f });
    vertices.push_back({ x + width, y + height, color[0], color[1], color[2], color[3], 1.0f, 1.0f });
    vertices.push_back({ x, y + height, color[0], color[1], color[2], color[3], 0.0f, 1.0f });

    // EBO
    indices.push_back(vertexCount + 0);
    indices.push_back(vertexCount + 1);
    indices.push_back(vertexCount + 2);

    indices.push_back(vertexCount + 0);
    indices.push_back(vertexCount + 2);
    indices.push_back(vertexCount + 3);
}