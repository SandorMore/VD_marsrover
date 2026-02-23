#pragma once
#include "raylib.h"
#include <cstdlib>

struct u_matrix {
    static constexpr size_t ROWS = 50;
    static constexpr size_t COLS = 50;

    char data[ROWS * COLS];

    char& operator()(size_t row, size_t col);
    const char& operator()(std::size_t row, std::size_t col) const;
};

Image load_img_for_icon();

void CoreLoop();