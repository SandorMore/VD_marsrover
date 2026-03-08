#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include "HeightMap.h"

class TerrainRenderer {
public:
    TerrainRenderer();
    ~TerrainRenderer();

    bool initialize(HeightMap* heightMap);
    void render(const glm::mat4& view, const glm::mat4& projection, bool showWireframe = false);
    void cleanup();

    void generateMesh();

    float getWidth() const { return mapWidth; }
    float getDepth() const { return mapDepth; }

private:
    HeightMap* map;
    int mapWidth;
    int mapDepth;

    GLuint VAO, VBO, EBO;
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec3> colors;
    std::vector<GLuint> indices;

    void createTerrainMesh();
    void setupBuffers();
};