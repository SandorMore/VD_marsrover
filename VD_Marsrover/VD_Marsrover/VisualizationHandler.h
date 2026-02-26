#pragma once
#include "GL/glew.h"
#include "GLFW/glfw3.h"

GLuint createShaderProgram();

void init(GLFWwindow* window);

void display(GLFWwindow* window, double currTime);

void printShaderLog(GLuint shader);

void prontProgramLog(GLuint program);

bool checkOpenGLError();