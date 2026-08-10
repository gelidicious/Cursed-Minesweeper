#ifndef UI_H
#define UI_H

#include <GLFW/glfw3.h>
#include <vector>

struct Vertex {
    float x, y;
    float r, g, b, a;
    float s, t;
};

class UIElement {
protected:
    float x, y;
    float width, height;
    float color[4];
public:
    UIElement(float x, float y, float width, float height, float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f);
    ~UIElement();

    void addVertices(std::vector<Vertex>& vertices, std::vector<GLuint>& indices);
};

#endif