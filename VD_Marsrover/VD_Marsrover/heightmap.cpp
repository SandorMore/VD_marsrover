#include "heightmap.h"
#include <cmath>
#include <vector>
#include <iostream>

static float Noise(int x, int y)
{
    int n = x + y * 57;
    n = (n << 13) ^ n;
    return 1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f;
}

static float SmoothNoise(float x, float y)
{
    int ix = (int)x;
    int iy = (int)y;

    float fx = x - ix;
    float fy = y - iy;

    float v00 = Noise(ix, iy);
    float v10 = Noise(ix + 1, iy);
    float v01 = Noise(ix, iy + 1);
    float v11 = Noise(ix + 1, iy + 1);

    float vx0 = v00 + (v10 - v00) * fx;
    float vx1 = v01 + (v11 - v01) * fx;

    return vx0 + (vx1 - vx0) * fy;
}

Image GenerateHeightmapImage(int width, int height, float scale, float heightMul, float offset)
{
    Image img = GenImageColor(width, height, BLACK);

    Color* pixels = LoadImageColors(img);

    for (int y = 0; y < height; y++)
        for (int x = 0; x < width; x++)
        {
            float n = SmoothNoise((x + offset) * scale, (y + offset) * scale);

            unsigned char v = (unsigned char)((n * 0.5f + 0.5f) * 255 * heightMul);

            pixels[x + y * width] = { v, v, v, 255 };
        }

    Image result = {
        pixels,
        width,
        height,
        1,
        PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
    };

    return result;
}