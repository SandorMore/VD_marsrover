#include "VisualizationHandler.h"


#define numVAOs 1

GLuint renderingProgram;
GLuint vao[numVAOs];


std::string readShaderSource(const char* filePath)
{
	std::string content;
	std::ifstream fileStream(filePath, std::ios::in);
	std::string line = "";

	while (!fileStream.eof()) {
		std::getline(fileStream, line);
		content.append(line + "\n");
	}
	fileStream.close();
	return content;
}

GLuint createShaderProgram() {

	GLuint vShader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);

	std::string vertShaderStr = readShaderSource("vertShaderSource.glsl");
	std::string fragShaderStr = readShaderSource("fragShaderSource.glsl");
	const char* vertShaderSrc = vertShaderStr.c_str();
	const char* fragShaderSrc = fragShaderStr.c_str();


	glShaderSource(vShader, 1, &vertShaderSrc, nullptr);
	glShaderSource(fShader, 1, &fragShaderSrc, nullptr);

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
	char* log = (char*)malloc(1);

	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
	if (len > 0) {
		log = (char*)realloc(log, len);
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

