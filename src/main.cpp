#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb/stb_image.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <algorithm>

#include "include/ui.h"
#include "include/texture.h"

const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in int aTexIndex;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec2 aOffset;

out vec2 texCoord;
flat out int texIndex;

void main() {
    gl_Position = vec4(aPos / 6 + aOffset, 0.0, 1.0);
    texCoord = aTexCoord;
    texIndex = aTexIndex;
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
in vec2 texCoord;
flat in int texIndex;

uniform sampler2D textures[32];
uniform vec2 mousePos;
uniform float flashlightRadius;

void main() {
    vec4 color = texture(textures[texIndex], texCoord);
    float pixelSize = 16;
    vec2 pixelPos = floor(gl_FragCoord.xy / pixelSize) * pixelSize;
    
    color.rgb *= max(1.0 - smoothstep(flashlightRadius * 0.7, flashlightRadius, distance(pixelPos, mousePos)), 0.05);

    FragColor = color;
}
)";

// -- Logic --
struct Tile {
    int number = 0;
    bool hidden = true;
};

const int MAX_MINES = 30;
const int LENGTH = 11;
const int DIRECTIONS[8][2] = {
    {-1,  1}, {0,  1}, {1,  1},
    {-1,  0},          {1,  0},
    {-1, -1}, {0, -1}, {1, -1},
};

int level = 0;
int mines = 10;
int corruptionChance = 1000;
float flashlightRadius = 1000.0f;

Tile board[LENGTH][LENGTH] = {};
int tilesLeft;
int firstClicked;

// -- Misc --
const int WIDTH = 512;  // 640;
const int HEIGHT = 512; // 480;

double mouseX = 0.0;
double mouseY = 0.0;

std::vector<int> textureChoices;
GLuint textureChoiceVBO;

std::vector<glm::vec2> transformations;

void resizeViewport(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// -- Function Prototypes --
void loadBoard();
void revealTile(int index);
void flagTile(int index);

void revealAdjacent(int index);
void addNumberToAdjacent(int index);
int getAdjacent(int index, int adjacentTiles[]);

void randomizeMines();

void setTexture(int index, int texture) {
    textureChoices[index] = texture;

    glBindBuffer(GL_ARRAY_BUFFER, textureChoiceVBO);
    glBufferSubData(GL_ARRAY_BUFFER, index * sizeof(int), sizeof(int), &textureChoices[index]);
}

void revealTile(int index) {
    if (textureChoices[index] == 11) // do not reveal flags
        return;

    int row = index / LENGTH;
    int column = index % LENGTH;

    if (board[row][column].hidden == false) // do not reveal already revealed tiles
        return;

    board[row][column].hidden = false;
    setTexture(index, board[row][column].number);

    if (firstClicked == -1) {   // generate mines after first clicking
        firstClicked = index;
        randomizeMines();
    }

    switch (board[row][column].number) {
    case 0:
        revealAdjacent(index);

        if (rand() % (2 * corruptionChance) == 0)
            setTexture(index, 12);
        else if (rand() % (8 * corruptionChance) == 0)
            setTexture(index, 13);
        else if (rand() % (8 * corruptionChance) == 0)
            setTexture(index, 14);
        else if (rand() % (128 * corruptionChance) == 0)
            setTexture(index, 15);
        break;
    case 1:
        if (rand() % (4 * corruptionChance) == 0)
            setTexture(index, 16);
        else if (rand() % (8 * corruptionChance) == 0)
            setTexture(index, 17);
        else if (rand() % (32 * corruptionChance) == 0)
            setTexture(index, 18);
        break;
    case 2:
        if (rand() % (4 * corruptionChance) == 0)
            setTexture(index, 19);
        else if (rand() % (8 * corruptionChance) == 0)
            setTexture(index, 20);
        else if (rand() % (8 * corruptionChance) == 0)
            setTexture(index, 21);
        break;
    case 3:
        if (rand() % (4 * corruptionChance) == 0)
            setTexture(index, 22);
        else if (rand() % (8 * corruptionChance) == 0)
            setTexture(index, 23);
        else if (rand() % (8 * corruptionChance) == 0)
            setTexture(index, 24);
        break;
    case 4:
        if (rand() % (4 * corruptionChance) == 0)
            setTexture(index, 25);
        else if (rand() % (8 * corruptionChance) == 0)
            setTexture(index, 26);
        break;
    case 5:
        if (rand() % (4 * corruptionChance) == 0)
            setTexture(index, 27);
        else if (rand() % (8 * corruptionChance) == 0)
            setTexture(index, 28);
        break;
    case 6:
        if (rand() % (4 * corruptionChance) == 0)
            setTexture(index, 29);
        else if (rand() % (8 * corruptionChance) == 0)
            setTexture(index, 30);
        break;
    case 7:
        if (rand() % (4 * corruptionChance) == 0)
            setTexture(index, 31);
        else if (rand() % (8 * corruptionChance) == 0)
            setTexture(index, 32);
        break;
    case 9:
        std::cout << "you lose..\n";    // lose condition
        break;
    }
    
    if (board[row][column].number >= 0 && board[row][column].number <= 8) {
        // win condition
        if (--tilesLeft <= 0) {
            loadBoard();
        }
    }
}

void flagTile(int index) {
    if (textureChoices[index] == 10) {      // flag
        textureChoices[index] = 11;
    }
    else if (textureChoices[index] == 11) { // unflag
        textureChoices[index] = 10;
    }
    else {
        return;
    }

    glBindBuffer(GL_ARRAY_BUFFER, textureChoiceVBO);
    glBufferSubData(GL_ARRAY_BUFFER, index * sizeof(int), sizeof(int), &textureChoices[index]);
}

int getAdjacent(int index, int adjacentTiles[]) {
    int i = 0;

    int row = index / LENGTH;
    int column = index % LENGTH;

    for (auto direction : DIRECTIONS) {
        int newRow = row + direction[1];
        int newColumn = column + direction[0];

        if (newRow < 0 || newRow >= LENGTH || newColumn < 0 || newColumn >= LENGTH)
            continue;

        int index = newRow * LENGTH + newColumn;

        adjacentTiles[i++] = index;
    }

    return i + 1;
}

void revealAdjacent(int index) {
    int row = index / LENGTH;
    int column = index % LENGTH;

    for (auto direction : DIRECTIONS) {
        int newRow = row + direction[1];
        int newColumn = column + direction[0];

        if (newRow < 0 || newRow >= LENGTH || newColumn < 0 || newColumn >= LENGTH)
            continue;

        int index = newRow * LENGTH + newColumn;
        revealTile(index);
    }
}

void addNumberToAdjacent(int index) {
    int row = index / LENGTH;
    int column = index % LENGTH;

    for (auto direction : DIRECTIONS) {
        int newRow = row + direction[1];
        int newColumn = column + direction[0];

        if (newRow < 0 || newRow >= LENGTH || newColumn < 0 || newColumn >= LENGTH)
            continue;
        if (board[newRow][newColumn].number == 9)
            continue;

        board[newRow][newColumn].number++;
    }
}

void randomizeMines() {
    int availableTiles[LENGTH * LENGTH];
    int tiles = 0;

    int adjacentTiles[8];
    int size = getAdjacent(firstClicked, adjacentTiles);

    for (int tile = 0; tile < LENGTH * LENGTH; tile++) {
        if (tile == firstClicked)                      // ignore the clicked tile
            continue;

        bool adjacent = false;

        for (int i = 0; i < size; i++) {
            if (tile == adjacentTiles[i]) {     // ignore the tiles surrounding the clicked tile
                adjacent = true;
                break;
            }
        }
        
        if (adjacent)
            continue;

        availableTiles[tiles++] = tile;
    }
    
    for (int i = 0; i < mines && tiles > 0; i++) {
        int randomIndex = std::rand() % tiles;
        int index = availableTiles[randomIndex];

        int row = index / LENGTH;
        int column = index % LENGTH;

        board[row][column].number = 9;

        addNumberToAdjacent(index);

        // set the last value to the removed index
        availableTiles[randomIndex] = availableTiles[tiles - 1];

        tiles--;
    }
}

void loadBoard() {
    // update stats
    level++;
    mines = std::min(LENGTH * LENGTH - 9, mines += rand() % 3 + 1);
    if (level >= 5)
        flashlightRadius = std::max(64.0f, 256.0f / (level - 4.0f));
    if (level >= 2)
        corruptionChance = std::floor(100 / (level * level * level) + 1);

    // display stats
    std::cout << "commencing board " << level << "\n";

    if (level >= 6)
        std::cout << mines * level << " mines" << "\n";
    else
        std::cout << mines << " mines" << "\n";

    // reset variables
    tilesLeft = LENGTH * LENGTH - mines;
    firstClicked = -1;

    // reset board
    for (auto& row : board) {
        for (Tile& tile : row) {
            tile.number = 0;
            tile.hidden = true;
        }
    }

    // reload textures
    for (int index = 0; index < LENGTH * LENGTH; index++) {
        textureChoices[index] = 10;

        glBindBuffer(GL_ARRAY_BUFFER, textureChoiceVBO);
        glBufferSubData(GL_ARRAY_BUFFER, index * sizeof(int), sizeof(int), &textureChoices[index]);
    }
}

void cursorMove(GLFWwindow* window, double x, double y) {
    mouseX = x;
    mouseY = HEIGHT - y;
}

void mouseButtonAction(GLFWwindow* window, int button, int action, int mods) {
    double x, y;
    glfwGetCursorPos(window, &x, &y);

    int width, height;
    glfwGetWindowSize(window, &width, &height);

    float ndcX = (2.0f * static_cast<float>(x)) / width - 1.0f;
    float ndcY = 1.0f - (2.0f * static_cast<float>(y)) / height;

    float halfSizeX = 0.5f / 6.0f;
    float halfSizeY = 0.5f / 6.0f;

    int index = 0;
    for (const auto& translation : transformations) {
        float centerX = translation.x;
        float centerY = translation.y;

        if (ndcX >= centerX - halfSizeX && ndcX <= centerX + halfSizeX &&
            ndcY >= centerY - halfSizeY && ndcY <= centerY + halfSizeY) {

            if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
                revealTile(index);
            else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
                flagTile(index);

            return;
        }
        index++;
    }
}

GLfloat vertices[] = {
      -0.5f, -0.5f,   0.0f, 0.0f,
       0.5f, -0.5f,   1.0f, 0.0f,
       0.5f,  0.5f,   1.0f, 1.0f,
      -0.5f,  0.5f,   0.0f, 1.0f,
};

GLuint indices[] = {
    0, 2, 1,
    0, 3, 2,
};

void generateArray(int count) {
    for (int y = -count / 2; y < count - count / 2; y++)
    {
        for (int x = -count / 2; x < count - count / 2; x++)
        {
            glm::vec2 translation;
            translation.x = static_cast<float>(x) / 6;
            translation.y = static_cast<float>(y) / 6;

            if (count % 2 == 0) {
                translation.x += 0.5f / 6;
                translation.y += 0.5f / 6;
            }

            transformations.push_back(translation);
        }
    }
}

int main() {
    GLFWwindow* window;

    // Init GLFW
    if (!glfwInit())
        return -1;

    // GLFW & OpenGL Version
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window
    window = glfwCreateWindow(WIDTH, HEIGHT, "minesweeper", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    // Make the window's context current
    glfwMakeContextCurrent(window);

    // Init GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        glfwTerminate();
        return -1;
    }

    // Resize viewport when window dimensions change
    glfwSetFramebufferSizeCallback(window, resizeViewport);

    // Allocate shader objects
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    // Create an empty program
    GLuint shaderProgram = glCreateProgram();

    // Attach shaders to program
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);

    // Convert shaders to a single executable
    glLinkProgram(shaderProgram);

    GLuint VAO, VBO, EBO, instanceVBO;

    // Generate object ID
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);                                             // Configs will be saved
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);      // Give GPU vertex data

    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Read positions
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Read textures
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // Generate instance VBO
    generateArray(LENGTH);

    textureChoices.resize(transformations.size(), 10);

    // Logic
    std::srand(time(NULL));

    loadBoard();

    //textureChoices[1] = 2;

    glGenBuffers(1, &instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, transformations.size() * sizeof(transformations[0]), transformations.data(), GL_STATIC_DRAW);

    // Read instances
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(3);
    glVertexAttribDivisor(3, 1);

    // Load Textures
    stbi_set_flip_vertically_on_load(true);

    // Add texture paths here
    Texture textures[] = {
        Texture{"textures/0.png"},
        Texture{"textures/1.png"},
        Texture{"textures/2.png"},
        Texture{"textures/3.png"},
        Texture{"textures/4.png"},
        Texture{"textures/5.png"},
        Texture{"textures/6.png"},
        Texture{"textures/7.png"},
        Texture{"textures/8.png"},
        Texture{"textures/mine.png"},
        Texture{"textures/hidden.png"}, // 10
        Texture{"textures/flag.png"},
        Texture{"textures/missingTextures.png"},
        Texture{"textures/0smudged.png"},
        Texture{"textures/i.png"},
        Texture{"textures/eye.png"},
        Texture{"textures/1smudged.png"},
        Texture{"textures/1smudged2.png"},
        Texture{"textures/phi.png"},
        Texture{"textures/2smudged.png"},
        Texture{"textures/2smudged2.png"}, // 20
        Texture{"textures/e.png"},
        Texture{"textures/3smudged.png"},
        Texture{"textures/3smudged2.png"},
        Texture{"textures/pi.png"},
        Texture{"textures/4smudged.png"},
        Texture{"textures/4smudged2.png"},
        Texture{"textures/5smudged.png"},
        Texture{"textures/5smudged2.png"},
        Texture{"textures/6smudged.png"},
        Texture{"textures/6smudged2.png"}, // 30
        Texture{"textures/7smudged.png"},
        Texture{"textures/7smudged2.png"},
    };

    short textureSize = sizeof(textures) / sizeof(textures[0]);
    
    // Set up units per texture
    for (int i = 0; i < textureSize; i++) {
        textures[i].unit(shaderProgram, ("textures[" + std::to_string(i) + "]").c_str(), i);
    }

    glGenBuffers(1, &textureChoiceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, textureChoiceVBO);
    glBufferData(GL_ARRAY_BUFFER, textureChoices.size() * sizeof(textureChoices[0]), textureChoices.data(), GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(1);
    glVertexAttribIPointer(1, 1, GL_INT, sizeof(int), (void*)0);
    glVertexAttribDivisor(1, 1);

    // Unbind objects
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Unrender occuded pixels
    glEnable(GL_DEPTH_TEST);

    // Blend transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Poll events
    glfwSetMouseButtonCallback(window, mouseButtonAction);
    glfwSetCursorPosCallback(window, cursorMove);

    // Calculate FPS
    double previousTime = 0.0;
    double currentTime = 0.0;
    double deltaTime;
    int frames = 0;

    // Loop window render
    while (!glfwWindowShouldClose(window)) {
        // Calculate FPS
        currentTime = glfwGetTime();
        deltaTime = currentTime - previousTime;
        frames++;

        if (deltaTime >= 1) {
            std::string fps = "minesweeper " + std::to_string(static_cast<int>(std::round((1.0 / deltaTime) * frames))) + " FPS";
            glfwSetWindowTitle(window, fps.c_str());
            previousTime = currentTime;
            frames = 0;
        }

        // Background color
        glClearColor(0.2f, 0.2f, 0.4f, 1.0f);

        // Clear screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Enable shader program
        glUseProgram(shaderProgram);

        // Flashlight
        glUniform2f(glGetUniformLocation(shaderProgram, "mousePos"), static_cast<float>(mouseX), static_cast<float>(mouseY));
        glUniform1f(glGetUniformLocation(shaderProgram, "flashlightRadius"), flashlightRadius);

        // Bind textures
        for (int i = 0; i < textureSize; i++) {
            textures[i].unitBind();
        }

        // Use the configurations as blueprint
        glBindVertexArray(VAO);

        // Draw on screen
        glDrawElementsInstanced(GL_TRIANGLES, sizeof(indices) / sizeof(GLuint), GL_UNSIGNED_INT, 0, transformations.size());

        // Swap front and back buffers
        glfwSwapBuffers(window);

        // Poll for and process events
        glfwPollEvents();
    }

    // Clear data
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    for (int i = 0; i < textureSize; i++) {
        textures[i].free();
    }
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}