#include "VisualizationHandler.h"
#include <cstdlib>
#include <iostream>

#define numVAOs 1

GLuint renderingProgram;
GLuint vao[numVAOs];

GLuint createShaderProgram() {
	
	const char* vshaderSource =
		"#version 430   \n"
		"void main(void) \n"
		"{gl_Position = vec4(0.0 ,0.0 ,0.0 ,1.0);}";

	const char* fshaderSource =
		"#version 430   \n"
		"out vec4 color;\n"
		"void main(void) \n"
		"{color = vec4(0.0 ,0.0 ,1.0 ,1.0);}";

	GLuint vShader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource(vShader, 1, &vshaderSource, nullptr);
	glShaderSource(fShader, 1, &fshaderSource, nullptr);

	glCompileShader(vShader);
	glCompileShader(fShader);

	GLuint vfprogram = glCreateProgram();
	
	glAttachShader(vfprogram, vShader);
	glAttachShader(vfprogram, fShader);

	glLinkProgram(vfprogram);

	return vfprogram;
}

void init(GLFWwindow* window) {
	renderingProgram = createShaderProgram();
	glGenVertexArrays(numVAOs, vao);
	glBindVertexArray(vao[0]);
}

void display(GLFWwindow* window, double currTime) {
	glUseProgram(renderingProgram);
	glPointSize(40);
	glDrawArrays(GL_POINTS, 0, 1);
}

void printShaderLog(GLuint shader)
{
	int len;
	int chrWritten;
	char* log;

	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
	if (len > 0) {
		log = (char*)malloc(len);
		glGetShaderInfoLog(shader, len, &chrWritten, log);
		std::cout << "Shader info: " << log << "\n";
		free(log);
	}
}

void prontProgramLog(GLuint program)
{
	int len;
	int chrWritten;
	char* log;

	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
	if (len > 0) {
		log = (char*)malloc(len);
		glGetProgramInfoLog(program, len, &chrWritten, log);
		std::cout << "Program info: " << log << "\n";
	}
	free(log);
}

bool checkOpenGLError()
{
	bool found = false;
	int glErr = glGetError();
	while (glErr != GL_NO_ERROR) {
		std::cout << glErr << "\n";
		found = true;
		glErr = glGetError();
	}
	return found;
}
