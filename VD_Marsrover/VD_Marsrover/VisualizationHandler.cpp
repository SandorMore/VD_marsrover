#include "VisualizationHandler.h"
#define numVAOs 1
#define numVBOs 2

int width, height;

GLuint renderingProgram;
GLuint vao[numVAOs];
GLuint vbo[numVBOs];

float aspect;

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
void processInput(GLFWwindow* window)
{

}
void setupVertices(void)
{
	float vertexPosition[108] =
	{
		// Back face (z = -1)
		-1.0f,  1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,

		 1.0f, -1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,

		// Front face (z = +1)
		-1.0f, -1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,

		 1.0f,  1.0f,  1.0f,
		 1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		// Left face (x = -1)
		-1.0f, -1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f,  1.0f,

		-1.0f,  1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f, -1.0f,

		// Right face (x = +1)
		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,

		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,

		 // Bottom face (y = -1)
		 -1.0f, -1.0f, -1.0f,
		 -1.0f, -1.0f,  1.0f,
		  1.0f, -1.0f,  1.0f,

		  1.0f, -1.0f,  1.0f,
		  1.0f, -1.0f, -1.0f,
		 -1.0f, -1.0f, -1.0f,

		 // Top face (y = +1)
		 -1.0f,  1.0f, -1.0f,
		  1.0f,  1.0f, -1.0f,
		  1.0f,  1.0f,  1.0f,

		  1.0f,  1.0f,  1.0f,
		 -1.0f,  1.0f,  1.0f,
		 -1.0f,  1.0f, -1.0f
	};

	glGenVertexArrays(1, vao);
	glBindVertexArray(vao[0]);
	glGenBuffers(numVBOs, vbo);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexPosition), vertexPosition, GL_STATIC_DRAW);
}

GLuint createShaderProgram(std::string vertShader, std::string fragShader) {

	GLuint vShader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);

	std::string vertShaderStr = readShaderSource(vertShader.c_str());
	std::string fragShaderStr = readShaderSource(fragShader.c_str());

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
	renderingProgram = createShaderProgram("vertShaderSource.glsl", "fragShaderSource.glsl");

}

void display(GLFWwindow* window, double currTime)
{

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
		log = ((char*)realloc(log, len) == nullptr) ? (char*)malloc(10) : nullptr;
		if (log) {
			glGetProgramInfoLog(program, len, &chrWritten, log);
			std::cout << "Program info: " << log << "\n";
		}
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


