#include "heightmap_mesh.h"
#include <cmath>
#include <cstring>
#include "raylib.h"
#include "raymath.h"

HeightmapMesh::HeightmapMesh(int size, float cellSize)
    : size(size), cellSize(cellSize)
{
    mesh = { 0 };
}

HeightmapMesh::~HeightmapMesh() {
    if (mesh.vertices) RL_FREE(mesh.vertices);
    if (mesh.normals) RL_FREE(mesh.normals);
    if (mesh.texcoords) RL_FREE(mesh.texcoords);
    if (mesh.indices) RL_FREE(mesh.indices);
}

void HeightmapMesh::GenerateMesh(float heightmap[][50]) {
    vertices.clear();
    normals.clear();
    texcoords.clear();
    indices.clear();

    // Center offset so mesh is around origin
    float half = (size - 1) * cellSize * 0.5f;

    // --- Build vertices, normals, texcoords ---
    for (int z = 0; z < size; z++) {
        for (int x = 0; x < size; x++) {
            vertices.push_back({
                x * cellSize - half,          // X centered
                heightmap[x][z],              // Y = height
                z * cellSize - half           // Z centered
                });
            normals.push_back({ 0, 1, 0 });    // temporary
            texcoords.push_back({
                x / float(size - 1),
                z / float(size - 1)
                });
        }
    }

    // --- Build indices (two triangles per quad) ---
    for (int z = 0; z < size - 1; z++) {
        for (int x = 0; x < size - 1; x++) {
            int topLeft = z * size + x;
            int topRight = topLeft + 1;
            int bottomLeft = topLeft + size;
            int bottomRight = bottomLeft + 1;

            // Triangle 1
            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            // Triangle 2
            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }

    // --- Allocate mesh memory ---
    mesh.vertexCount = vertices.size();
    mesh.triangleCount = indices.size() / 3;

    mesh.vertices = (float*)RL_MALLOC(vertices.size() * 3 * sizeof(float));
    mesh.normals = (float*)RL_MALLOC(vertices.size() * 3 * sizeof(float));
    mesh.texcoords = (float*)RL_MALLOC(vertices.size() * 2 * sizeof(float));
    mesh.indices = (unsigned short*)RL_MALLOC(indices.size() * sizeof(unsigned short));

    // Copy vertex data
    for (size_t i = 0; i < vertices.size(); i++) {
        mesh.vertices[i * 3 + 0] = vertices[i].x;
        mesh.vertices[i * 3 + 1] = vertices[i].y;
        mesh.vertices[i * 3 + 2] = vertices[i].z;

        mesh.normals[i * 3 + 0] = normals[i].x;
        mesh.normals[i * 3 + 1] = normals[i].y;
        mesh.normals[i * 3 + 2] = normals[i].z;
    }

    for (size_t i = 0; i < texcoords.size(); i++) {
        mesh.texcoords[i * 2 + 0] = texcoords[i].x;
        mesh.texcoords[i * 2 + 1] = texcoords[i].y;
    }

    memcpy(mesh.indices, indices.data(), indices.size() * sizeof(unsigned short));

    // --- Calculate correct normals ---
    CalculateNormals();
}

void HeightmapMesh::CalculateNormals() {
    // Reset normals
    for (size_t i = 0; i < mesh.vertexCount * 3; i++)
        mesh.normals[i] = 0.0f;

    // For each triangle
    for (size_t i = 0; i < indices.size(); i += 3) {
        unsigned short i0 = indices[i];
        unsigned short i1 = indices[i + 1];
        unsigned short i2 = indices[i + 2];

        Vector3 v0 = { mesh.vertices[i0 * 3 + 0], mesh.vertices[i0 * 3 + 1], mesh.vertices[i0 * 3 + 2] };
        Vector3 v1 = { mesh.vertices[i1 * 3 + 0], mesh.vertices[i1 * 3 + 1], mesh.vertices[i1 * 3 + 2] };
        Vector3 v2 = { mesh.vertices[i2 * 3 + 0], mesh.vertices[i2 * 3 + 1], mesh.vertices[i2 * 3 + 2] };

        Vector3 edge1 = Vector3Subtract(v1, v0);
        Vector3 edge2 = Vector3Subtract(v2, v0);
        Vector3 normal = Vector3CrossProduct(edge1, edge2);

        // Add this triangle's normal to each vertex
        mesh.normals[i0 * 3 + 0] += normal.x;
        mesh.normals[i0 * 3 + 1] += normal.y;
        mesh.normals[i0 * 3 + 2] += normal.z;

        mesh.normals[i1 * 3 + 0] += normal.x;
        mesh.normals[i1 * 3 + 1] += normal.y;
        mesh.normals[i1 * 3 + 2] += normal.z;

        mesh.normals[i2 * 3 + 0] += normal.x;
        mesh.normals[i2 * 3 + 1] += normal.y;
        mesh.normals[i2 * 3 + 2] += normal.z;
    }

    // Normalize all normals
    for (size_t i = 0; i < mesh.vertexCount; i++) {
        Vector3 n = { mesh.normals[i * 3 + 0], mesh.normals[i * 3 + 1], mesh.normals[i * 3 + 2] };
        n = Vector3Normalize(n);
        mesh.normals[i * 3 + 0] = n.x;
        mesh.normals[i * 3 + 1] = n.y;
        mesh.normals[i * 3 + 2] = n.z;
    }
}

Mesh& HeightmapMesh::GetMesh() {
    return mesh;
}