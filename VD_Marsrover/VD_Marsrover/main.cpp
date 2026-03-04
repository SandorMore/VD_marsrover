#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#define GLM_ENABLE_EXPERIMENTAL
#include "Camera.h"
#include "HeightMap.h"
#include "TerrainRenderer.h"
#include "rover.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window, float deltaTime);
void updateDayNightCycle(float deltaTime);

Camera camera(glm::vec3(25.0f, 20.0f, 25.0f));
float lastX = 400.0f;
float lastY = 300.0f;
bool firstMouse = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

HeightMap g_heightMap;
TerrainRenderer g_terrainRenderer;

// Rover simulation / visualization state
std::vector<std::vector<Cell>> g_roverMap(MAP_SIZE, std::vector<Cell>(MAP_SIZE));
Position g_roverStart;
std::vector<Position> g_roverPath;
size_t g_roverPathIndex = 0;
float g_roverAnimTime = 0.0f;
const float g_roverStepDuration = 0.1f; // seconds per path step

GLuint g_roverVAO = 0;
GLuint g_roverVBO = 0;
GLuint g_roverEBO = 0;

float g_timeOfDay = 6.0f;
bool g_isDay = true;
glm::vec3 g_ambientLight = glm::vec3(0.8f, 0.8f, 0.7f);
glm::vec3 g_sunColor = glm::vec3(1.0f, 0.95f, 0.9f);

bool g_showWireframe = false;
bool g_showGrid = true;
float g_terrainScale = 1.0f;

GLuint shaderProgram;

// Simple white quad used to visualize the rover as a box
void setupRoverMesh() {
    if (g_roverVAO != 0) return;

    float size = 0.4f;
    float y = 0.0f;

    // position (3) + color (3), white color
    float vertices[] = {
        -size, y, -size,  1.0f, 1.0f, 1.0f,
         size, y, -size,  1.0f, 1.0f, 1.0f,
         size, y,  size,  1.0f, 1.0f, 1.0f,
        -size, y,  size,  1.0f, 1.0f, 1.0f
    };

    unsigned int indices[] = {
        0, 1, 2,
        0, 2, 3
    };

    glGenVertexArrays(1, &g_roverVAO);
    glGenBuffers(1, &g_roverVBO);
    glGenBuffers(1, &g_roverEBO);

    glBindVertexArray(g_roverVAO);

    glBindBuffer(GL_ARRAY_BUFFER, g_roverVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_roverEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void cleanupRoverMesh() {
    if (g_roverVAO) {
        glDeleteVertexArrays(1, &g_roverVAO);
        glDeleteBuffers(1, &g_roverVBO);
        glDeleteBuffers(1, &g_roverEBO);
        g_roverVAO = g_roverVBO = g_roverEBO = 0;
    }
}

int main(int argc, char** argv) {
    // Default map file is asd.txt, unless overridden by command-line argument
    std::string mapFile = "asd.txt";
    if (argc > 1) {
        mapFile = argv[1];
    }

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1200, 800, "Mars Terrain Visualization", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        glfwTerminate();
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    std::cout << "Loading map: " << mapFile << std::endl;

    if (!g_heightMap.loadFromCSV(mapFile)) {
        std::cerr << "Failed to load map '" << mapFile
                  << "'. Creating default 50x50 map." << std::endl;

        std::ofstream defaultMap(mapFile);
        if (!defaultMap.is_open()) {
            std::cerr << "Could not create default map file '" << mapFile << "'. Exiting."
                      << std::endl;
            glfwTerminate();
            return -1;
        }

        for (int i = 0; i < 50; i++) {
            for (int j = 0; j < 50; j++) {
                if (i == 25 && j == 25) defaultMap << 'S';
                else if (rand() % 10 == 0) defaultMap << 'B';
                else if (rand() % 15 == 0) defaultMap << 'Y';
                else if (rand() % 20 == 0) defaultMap << 'G';
                else if (rand() % 8 == 0) defaultMap << '#';
                else defaultMap << '.';
            }
            defaultMap << '\n';
        }
        defaultMap.close();

        if (!g_heightMap.loadFromCSV(mapFile)) {
            std::cerr << "Failed to load generated default map from '" << mapFile
                      << "'. Exiting." << std::endl;
            glfwTerminate();
            return -1;
        }
    }

    g_heightMap.generateHeights(0.0f, 0.5f);

    if (!g_terrainRenderer.initialize(&g_heightMap)) {
        std::cerr << "Failed to initialize terrain renderer" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Prepare rover logical map and build a fast A* based route using the same file
    if (!readMap(mapFile, g_roverMap, g_roverStart)) {
        std::cerr << "Failed to read rover map from '" << mapFile << "'" << std::endl;
    } else {
        g_roverPath = buildFastRoute(g_roverMap, g_roverStart);
        // Apply a simple time limit: 24 hours.
        // One step corresponds to one half-hour slot, so keep at most 24 * 2 = 48 steps.
        const int maxStepsFor24Hours = 24 * 2;
        if (g_roverPath.size() > static_cast<size_t>(maxStepsFor24Hours)) {
            g_roverPath.resize(maxStepsFor24Hours);
        }

        std::cout << "Fast A* route length (steps, capped to 24h): " << g_roverPath.size() << std::endl;
    }

    // Setup rover mesh for visualization
    setupRoverMesh();

    const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aColor;
        
        out vec3 ourColor;
        out vec3 FragPos;
        
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        uniform float timeOfDay;
        
        void main() {
            FragPos = vec3(model * vec4(aPos, 1.0));
            gl_Position = projection * view * model * vec4(aPos, 1.0);
            ourColor = aColor;
        }
    )";

    const char* fragmentShaderSource = R"(
        #version 330 core
        in vec3 ourColor;
        in vec3 FragPos;
        out vec4 FragColor;
        
        uniform vec3 ambientLight;
        uniform vec3 sunColor;
        uniform bool isDay;
        uniform float timeOfDay;
        
        void main() {
            vec3 color = ourColor;
            
            // Simple lighting based on time of day
            float intensity = isDay ? 1.0 : 0.4;
            
            if (!isDay) {
                // Night time - add blue tint
                color = mix(color, vec3(0.2, 0.2, 0.6), 0.3);
            }
            
            // Add dawn/dusk effects
            float dawnFactor = 1.0 - abs(timeOfDay - 6.0) / 6.0;
            if (timeOfDay > 4.0 && timeOfDay < 8.0) {
                // Dawn/dusk - add orange/red tint
                color = mix(color, vec3(1.0, 0.5, 0.2), dawnFactor * 0.5);
            }
            
            // Simple directional lighting based on position
            vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
            float diff = max(dot(vec3(0.0, 1.0, 0.0), lightDir), 0.0);
            
            FragColor = vec4(color * (intensity + diff * 0.3), 1.0);
        }
    )";

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);

    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "Vertex shader compilation failed: " << infoLog << std::endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "Fragment shader compilation failed: " << infoLog << std::endl;
    }

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "Shader program linking failed: " << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    std::cout << "\n=== Mars Terrain Loaded ===" << std::endl;
    std::cout << "Map size: " << g_heightMap.getWidth() << " x " << g_heightMap.getHeight() << std::endl;
    std::cout << "Starting position: (" << g_heightMap.getStartPosition().x << ", " << g_heightMap.getStartPosition().y << ")" << std::endl;

    auto minerals = g_heightMap.getMineralPositions();
    std::cout << "Total minerals: " << minerals.size() << std::endl;
    std::cout << "  Blue (B): " << g_heightMap.countMineralType(TerrainType::BLUE_MINERAL) << std::endl;
    std::cout << "  Yellow (Y): " << g_heightMap.countMineralType(TerrainType::YELLOW_MINERAL) << std::endl;
    std::cout << "  Green (G): " << g_heightMap.countMineralType(TerrainType::GREEN_MINERAL) << std::endl;
    std::cout << "===========================\n" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  WASD - Move camera" << std::endl;
    std::cout << "  Mouse - Look around" << std::endl;
    std::cout << "  Scroll - Zoom in/out" << std::endl;
    std::cout << "  F1 - Toggle wireframe mode" << std::endl;
    std::cout << "  F2 - Reset camera to top view" << std::endl;
    std::cout << "  F3 - Toggle day/night cycle" << std::endl;
    std::cout << "  ESC - Exit" << std::endl;
    std::cout << "===========================\n" << std::endl;

    float simulationSpeed = 1.0f;
    bool isPaused = false;

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window, deltaTime);

        if (!isPaused) {
            updateDayNightCycle(deltaTime * simulationSpeed);
        }

        glm::vec3 skyColor;
        if (g_isDay) {
            skyColor = glm::vec3(0.5f, 0.7f, 1.0f) * 0.8f; 
        }
        else {
            skyColor = glm::vec3(0.05f, 0.05f, 0.2f); 
        }

        if (g_timeOfDay > 4.0f && g_timeOfDay < 8.0f) {
            float t = (g_timeOfDay - 4.0f) / 4.0f;
            skyColor = glm::mix(glm::vec3(0.7f, 0.4f, 0.2f), glm::vec3(0.5f, 0.7f, 1.0f), t);
        }
        else if (g_timeOfDay > 16.0f && g_timeOfDay < 20.0f) {
            float t = (g_timeOfDay - 16.0f) / 4.0f;
            skyColor = glm::mix(glm::vec3(0.5f, 0.7f, 1.0f), glm::vec3(0.2f, 0.1f, 0.3f), t);
        }

        glClearColor(skyColor.r, skyColor.g, skyColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), 1200.0f / 800.0f, 0.1f, 200.0f);
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 terrainModel = glm::mat4(1.0f);

        float terrainWidth = g_terrainRenderer.getWidth();
        float terrainDepth = g_terrainRenderer.getDepth();
        terrainModel = glm::translate(terrainModel, glm::vec3(-terrainWidth / 2.0f, 0.0f, -terrainDepth / 2.0f));
        terrainModel = glm::scale(terrainModel, glm::vec3(g_terrainScale));

        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, &view[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &terrainModel[0][0]);

        glUniform3fv(glGetUniformLocation(shaderProgram, "ambientLight"), 1, &g_ambientLight[0]);
        glUniform3fv(glGetUniformLocation(shaderProgram, "sunColor"), 1, &g_sunColor[0]);
        glUniform1i(glGetUniformLocation(shaderProgram, "isDay"), g_isDay);
        glUniform1f(glGetUniformLocation(shaderProgram, "timeOfDay"), g_timeOfDay);

        // Draw terrain
        g_terrainRenderer.render(view, projection, g_showWireframe);

        // Animate rover along planned path as a small white box
        if (!g_roverPath.empty()) {
            g_roverAnimTime += deltaTime;
            while (g_roverAnimTime >= g_roverStepDuration && g_roverPathIndex + 1 < g_roverPath.size()) {
                g_roverAnimTime -= g_roverStepDuration;
                g_roverPathIndex++;
            }

            const Position& p = g_roverPath[g_roverPathIndex];

            // The rover logic uses (x,y) as map indices; map[row][col] with row=x, col=y.
            // Map that to terrain local coordinates: x -> z, y -> x so it aligns with HeightMap.
            float cellX = static_cast<float>(p.y);
            float cellZ = static_cast<float>(p.x);
            float cellHeight = g_heightMap.getHeightAt(cellX, cellZ) + 0.3f;

            glm::mat4 roverModel = terrainModel;
            roverModel = glm::translate(roverModel, glm::vec3(cellX, cellHeight, cellZ));

            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &roverModel[0][0]);

            glBindVertexArray(g_roverVAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }

        // Render simple coordinate grid if enabled
        if (g_showGrid) {
            // Simple grid rendering would go here
            // You could add grid lines to show cell boundaries
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    g_terrainRenderer.cleanup();
    cleanupRoverMesh();
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window, float deltaTime) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(CameraMovement::FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(CameraMovement::BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(CameraMovement::LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(CameraMovement::RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        camera.ProcessKeyboard(CameraMovement::UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        camera.ProcessKeyboard(CameraMovement::DOWN, deltaTime);


    static bool f1Pressed = false;
    if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS && !f1Pressed) {
        g_showWireframe = !g_showWireframe;
        f1Pressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_RELEASE) {
        f1Pressed = false;
    }

    static bool f2Pressed = false;
    if (glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS && !f2Pressed) {
        camera.Position = glm::vec3(25.0f, 40.0f, 25.0f);
        camera.Yaw = -90.0f;
        camera.Pitch = -45.0f;
        camera.updateCameraVectors();
        f2Pressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_F2) == GLFW_RELEASE) {
        f2Pressed = false;
    }

    static bool f3Pressed = false;
    if (glfwGetKey(window, GLFW_KEY_F3) == GLFW_PRESS && !f3Pressed) {
        g_showGrid = !g_showGrid;
        f3Pressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_F3) == GLFW_RELEASE) {
        f3Pressed = false;
    }

    if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_KP_ADD) == GLFW_PRESS) {
    }

    if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS) {
    }
}

void updateDayNightCycle(float deltaTime) {
    g_timeOfDay += deltaTime * 0.5f;

    if (g_timeOfDay >= 24.0f) {
        g_timeOfDay = 0.0f;
    }

    g_isDay = (g_timeOfDay >= 6.0f && g_timeOfDay < 22.0f);

    if (g_isDay) {
        float dayProgress = (g_timeOfDay - 6.0f) / 16.0f; 
        g_ambientLight = glm::vec3(0.8f, 0.8f, 0.7f) * (0.7f + 0.3f * sin(dayProgress * 3.14159f));
    }
    else {
        float nightProgress = (g_timeOfDay - 22.0f) / 8.0f;
        if (g_timeOfDay < 6.0f) {
            nightProgress = (g_timeOfDay + 2.0f) / 8.0f;
        }
        g_ambientLight = glm::vec3(0.2f, 0.2f, 0.3f) * (0.5f + 0.5f * sin(nightProgress * 3.14159f));
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xOffset = xpos - lastX;
    float yOffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xOffset, yOffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    camera.ProcessMouseScroll(yoffset);
}