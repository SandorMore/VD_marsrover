#include <iostream>
#include "raylib.h"
#include "misc.h"

#define WIDTH 800
#define HEIGHT 600
#define TARGET_FPS 60

int main()
{
    InitWindow(WIDTH, HEIGHT, "Hello Raylib");

    Image icon = load_img_for_icon();
    SetWindowIcon(icon);

    SetTargetFPS(TARGET_FPS);

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(GRAY);
        DrawText("Hello Raylib!", 190, 200, 20, GREEN);
        EndDrawing();
    }

    UnloadImage(icon);

    CloseWindow();
}