#pragma once
#include "raylib.h"
#include "rover.h"
#include <vector>

extern const float CELL_SIZE;

Vector3 gridToWorld(const Position& p);

Position worldToGrid(const Vector3& v);