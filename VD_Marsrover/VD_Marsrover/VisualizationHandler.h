#pragma once
#include "GL/glew.h"
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <fstream>
#include <string>
#include <cmath>

void setupVertices(void);

GLuint createShaderProgram(std::string vertShader, std::string fragShader);

void init(GLFWwindow* window);

void display(GLFWwindow* window, double currTime);

void printShaderLog(GLuint shader);

void prontProgramLog(GLuint program);

bool checkOpenGLError();

std::string readShaderSource(const char* filePath);

void processInput(GLFWwindow* window);
