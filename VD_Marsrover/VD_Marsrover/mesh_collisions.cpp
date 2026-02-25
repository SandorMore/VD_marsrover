#include "rover.h"
#include <iostream>
#include <string>
#include <cstdlib>
#include <ctime>
#include <thread>
#include "misc.h"
#include "raymath.h"
#include "heightmap.h"
#include <vector>
#include <cstdint>
#include <cstdio>

float GetHeightAt(Mesh mesh, Matrix transform, Vector3 position)
{
    Ray ray;

    ray.position = { position.x, -1000.0f, position.z };
    ray.direction = { 0, 1, 0 };

    RayCollision hit = GetRayCollisionMesh(ray, mesh, transform);

    if (hit.hit)
        return hit.point.y;

    return 0.0f;
}