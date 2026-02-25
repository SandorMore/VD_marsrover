#pragma once

#include <vector>
#include "raylib.h"
#include "rover.h"

void main_loop(
    const char* title,
    const std::vector<std::vector<Cell>>& map,
    const Position& startPos,
    const std::vector<Position>& route
);