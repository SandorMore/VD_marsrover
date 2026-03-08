#include "TerrainRenderer.h"
#include <iostream>

TerrainRenderer::TerrainRenderer() : map(nullptr), VAO(0), VBO(0), EBO(0) {}

TerrainRenderer::~TerrainRenderer() {
    cleanup();
}

bool TerrainRenderer::initialize(HeightMap* heightMap) {
    map = heightMap;
    if (!map) {
        std::cerr << "No height map provided" << std::endl;
        return false;
    }

    mapWidth = map->getWidth();
    mapDepth = map->getHeight();

    generateMesh();
    setupBuffers();

    return true;
}

void TerrainRenderer::generateMesh() {
    createTerrainMesh();
}

void TerrainRenderer::createTerrainMesh() {
    vertices.clear();
    normals.clear();
    colors.clear();
    indices.clear();

    float cellSize = 1.0f;

    // Helper to add a single rectangular block covering [x0,x1]x[z0,z1] from bottom to top.
    auto addBlock = [&](float x0, float x1, float z0, float z1, float bottom, float top, const glm::vec3& color) {
        GLuint baseIndex = static_cast<GLuint>(vertices.size());

        const glm::vec3 nTop(0.0f, 1.0f, 0.0f);
        const glm::vec3 nBottom(0.0f, -1.0f, 0.0f);
        const glm::vec3 nFront(0.0f, 0.0f, -1.0f);
        const glm::vec3 nBack(0.0f, 0.0f, 1.0f);
        const glm::vec3 nLeft(-1.0f, 0.0f, 0.0f);
        const glm::vec3 nRight(1.0f, 0.0f, 0.0f);

        // Top face
        vertices.push_back(glm::vec3(x0, top, z0)); normals.push_back(nTop); colors.push_back(color);
        vertices.push_back(glm::vec3(x1, top, z0)); normals.push_back(nTop); colors.push_back(color);
        vertices.push_back(glm::vec3(x1, top, z1)); normals.push_back(nTop); colors.push_back(color);
        vertices.push_back(glm::vec3(x0, top, z1)); normals.push_back(nTop); colors.push_back(color);

        // Bottom face
        vertices.push_back(glm::vec3(x0, bottom, z0)); normals.push_back(nBottom); colors.push_back(color);
        vertices.push_back(glm::vec3(x1, bottom, z0)); normals.push_back(nBottom); colors.push_back(color);
        vertices.push_back(glm::vec3(x1, bottom, z1)); normals.push_back(nBottom); colors.push_back(color);
        vertices.push_back(glm::vec3(x0, bottom, z1)); normals.push_back(nBottom); colors.push_back(color);

        // Front face (-Z)
        vertices.push_back(glm::vec3(x0, bottom, z0)); normals.push_back(nFront); colors.push_back(color);
        vertices.push_back(glm::vec3(x1, bottom, z0)); normals.push_back(nFront); colors.push_back(color);
        vertices.push_back(glm::vec3(x1, top,   z0)); normals.push_back(nFront); colors.push_back(color);
        vertices.push_back(glm::vec3(x0, top,   z0)); normals.push_back(nFront); colors.push_back(color);

        // Back face (+Z)
        vertices.push_back(glm::vec3(x0, bottom, z1)); normals.push_back(nBack); colors.push_back(color);
        vertices.push_back(glm::vec3(x1, bottom, z1)); normals.push_back(nBack); colors.push_back(color);
        vertices.push_back(glm::vec3(x1, top,   z1)); normals.push_back(nBack); colors.push_back(color);
        vertices.push_back(glm::vec3(x0, top,   z1)); normals.push_back(nBack); colors.push_back(color);

        // Left face (-X)
        vertices.push_back(glm::vec3(x0, bottom, z0)); normals.push_back(nLeft); colors.push_back(color);
        vertices.push_back(glm::vec3(x0, bottom, z1)); normals.push_back(nLeft); colors.push_back(color);
        vertices.push_back(glm::vec3(x0, top,   z1)); normals.push_back(nLeft); colors.push_back(color);
        vertices.push_back(glm::vec3(x0, top,   z0)); normals.push_back(nLeft); colors.push_back(color);

        // Right face (+X)
        vertices.push_back(glm::vec3(x1, bottom, z0)); normals.push_back(nRight); colors.push_back(color);
        vertices.push_back(glm::vec3(x1, bottom, z1)); normals.push_back(nRight); colors.push_back(color);
        vertices.push_back(glm::vec3(x1, top,   z1)); normals.push_back(nRight); colors.push_back(color);
        vertices.push_back(glm::vec3(x1, top,   z0)); normals.push_back(nRight); colors.push_back(color);

        // Top
        indices.push_back(baseIndex + 0);
        indices.push_back(baseIndex + 1);
        indices.push_back(baseIndex + 2);
        indices.push_back(baseIndex + 0);
        indices.push_back(baseIndex + 2);
        indices.push_back(baseIndex + 3);

        // Bottom
        indices.push_back(baseIndex + 4);
        indices.push_back(baseIndex + 6);
        indices.push_back(baseIndex + 5);
        indices.push_back(baseIndex + 4);
        indices.push_back(baseIndex + 7);
        indices.push_back(baseIndex + 6);

        // Front
        indices.push_back(baseIndex + 8);
        indices.push_back(baseIndex + 9);
        indices.push_back(baseIndex + 10);
        indices.push_back(baseIndex + 8);
        indices.push_back(baseIndex + 10);
        indices.push_back(baseIndex + 11);

        // Back
        indices.push_back(baseIndex + 12);
        indices.push_back(baseIndex + 14);
        indices.push_back(baseIndex + 13);
        indices.push_back(baseIndex + 12);
        indices.push_back(baseIndex + 15);
        indices.push_back(baseIndex + 14);

        // Left
        indices.push_back(baseIndex + 16);
        indices.push_back(baseIndex + 18);
        indices.push_back(baseIndex + 17);
        indices.push_back(baseIndex + 16);
        indices.push_back(baseIndex + 19);
        indices.push_back(baseIndex + 18);

        // Right
        indices.push_back(baseIndex + 20);
        indices.push_back(baseIndex + 21);
        indices.push_back(baseIndex + 22);
        indices.push_back(baseIndex + 20);
        indices.push_back(baseIndex + 22);
        indices.push_back(baseIndex + 23);
    };

    // Track which obstacle cells are already merged into a larger block.
    std::vector<std::vector<bool>> obstacleUsed(
        mapDepth, std::vector<bool>(mapWidth, false));

    for (int z = 0; z < mapDepth; z++) {
        for (int x = 0; x < mapWidth; x++) {
            TerrainCell cell = map->getCell(x, z);

            if (cell.type == TerrainType::OBSTACLE) {
                if (obstacleUsed[z][x]) continue;

                // Grow a maximal rectangle of adjacent obstacles starting at (x,z).
                int xEnd = x;
                while (xEnd < mapWidth &&
                       map->getCell(xEnd, z).type == TerrainType::OBSTACLE &&
                       !obstacleUsed[z][xEnd]) {
                    xEnd++;
                }

                int zEnd = z + 1;
                bool canExpand = true;
                while (zEnd < mapDepth && canExpand) {
                    for (int xi = x; xi < xEnd; ++xi) {
                        if (map->getCell(xi, zEnd).type != TerrainType::OBSTACLE ||
                            obstacleUsed[zEnd][xi]) {
                            canExpand = false;
                            break;
                        }
                    }
                    if (canExpand) {
                        zEnd++;
                    }
                }

                // Create one big block for this rectangle.
                float top = cell.height;
                float bottom = 0.0f;
                glm::vec3 color = cell.color;

                float x0 = x * cellSize;
                float x1 = xEnd * cellSize;
                float z0 = z * cellSize;
                float z1 = zEnd * cellSize;

                addBlock(x0, x1, z0, z1, bottom, top, color);

                for (int zz = z; zz < zEnd; ++zz) {
                    for (int xx = x; xx < xEnd; ++xx) {
                        obstacleUsed[zz][xx] = true;
                    }
                }
            } else {
                // Non-obstacle cells remain as individual 1x1 columns.
                float top = cell.height;
                float bottom = 0.0f;
                glm::vec3 color = cell.color;

                float x0 = x * cellSize;
                float x1 = (x + 1) * cellSize;
                float z0 = z * cellSize;
                float z1 = (z + 1) * cellSize;

                addBlock(x0, x1, z0, z1, bottom, top, color);
            }
        }
    }
}

void TerrainRenderer::setupBuffers() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    std::vector<float> interleavedData;
    for (size_t i = 0; i < vertices.size(); i++) {

        interleavedData.push_back(vertices[i].x);
        interleavedData.push_back(vertices[i].y);
        interleavedData.push_back(vertices[i].z);

        interleavedData.push_back(normals[i].x);
        interleavedData.push_back(normals[i].y);
        interleavedData.push_back(normals[i].z);

        interleavedData.push_back(colors[i].x);
        interleavedData.push_back(colors[i].y);
        interleavedData.push_back(colors[i].z);
    }

    glBufferData(GL_ARRAY_BUFFER, interleavedData.size() * sizeof(float),
        interleavedData.data(), GL_STATIC_DRAW);

    // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float),
        (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // color
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float),
        (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint),
        indices.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);
}

void TerrainRenderer::render(const glm::mat4& view, const glm::mat4& projection, bool showWireframe) {
    if (VAO == 0) return;

    glBindVertexArray(VAO);

    if (showWireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glBindVertexArray(0);
}

void TerrainRenderer::cleanup() {
    if (VAO) {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
        VAO = 0;
        VBO = 0;
        EBO = 0;
    }
}