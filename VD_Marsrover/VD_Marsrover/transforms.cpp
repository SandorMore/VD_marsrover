#include "transforms.h"

const float CELL_SIZE = 1.0f;


Vector3 gridToWorld(const Position& p)
{
    float offset = MAP_SIZE * CELL_SIZE * 0.5f;

    return {
        p.x * CELL_SIZE - offset,
        0.0f,
        p.y * CELL_SIZE - offset
    };
}


Position worldToGrid(const Vector3& v)
{
    float offset = MAP_SIZE * CELL_SIZE * 0.5f;

    return Position(
        (int)((v.x + offset) / CELL_SIZE),
        (int)((v.z + offset) / CELL_SIZE)
    );
}