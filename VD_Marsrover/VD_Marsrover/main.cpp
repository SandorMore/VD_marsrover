#include "rover.h"
#include "GL/glew.h"
#include "GLFW/glfw3.h"
#include "VisualizationHandler.h"

#include <vector>
#include <string>
#include <iostream>
#include <thread>

#define WIDTH 1280
#define HEIGHT 720

int main()
{
    std::string map_file = "asd.txt";
    int max_half_hours = 200;

    std::vector<std::vector<Cell>> map(MAP_SIZE, std::vector<Cell>(MAP_SIZE));
    Position start_position;

    if (!readMap(map_file, map, start_position))
    {
        std::cout << "Map load failed\n";
        return 1;
    }

    auto route = buildFastRoute(map, start_position);

    if (!glfwInit()) exit(EXIT_FAILURE);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Marsrover", nullptr, nullptr);
    if (!window) {
        std::cout << "Window creation failed\n";
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK) {
        std::cout << "GLEW init failed\n";
        return 1;
    }

    glfwSwapInterval(1);

    // Initialize visualization
    init(window);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);
        display(window, glfwGetTime());
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}