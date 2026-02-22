#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include "misc.h"

char& u_matrix::operator()(size_t row, size_t col)
{
    return data[row * COLS + col];
}

const char& u_matrix::operator()(size_t row, size_t col) const
{
    return data[row * COLS + col];
}
