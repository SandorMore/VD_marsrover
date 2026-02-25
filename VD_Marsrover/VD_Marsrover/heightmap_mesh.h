#pragma once
#include "raylib.h"
#include <vector>

class HeightmapMesh {
public:
    HeightmapMesh(int size, float cellSize);
    ~HeightmapMesh();

    void GenerateMesh(float heightmap[][50]);
    Mesh& GetMesh();

private:
    int size;
    float cellSize;
    Mesh mesh;
    std::vector<Vector3> vertices;
    std::vector<Vector3> normals;
    std::vector<Vector2> texcoords;
    std::vector<unsigned short> indices;

    void CalculateNormals();
};