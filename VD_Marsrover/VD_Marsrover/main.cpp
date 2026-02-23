#include "rover.h"
#include <iostream>
#include <string>
#include <cstdlib>
#include <ctime>
#include <thread>
#include "misc.h"

#define WIDTH 1200
#define HEIGHT 800
#define TARGET_FPS 60

using namespace std;

//rover_log_1771854750.csv

#define MAX_HOURS_TSET 100

void CoreLoop(const char* title);

int main(int argc, char* argv[]) {
    string mapFile = R"(C:\Users\Sanyi\Desktop\verseny\VD_Marsrover\VD_Marsrover\asd.txt)";
    int maxHours = MAX_HOURS_TSET; //atoi(argv[2])
    int maxHalfHours = maxHours * 2;

    std::thread t();
    CoreLoop("Marsrover");


    /*if (t.joinable()) {
        t.join();
    }
    return 0;*/
}
const float CELL_SIZE = 1.0f;

void CoreLoop(const char* title)
{
    InitWindow(WIDTH, HEIGHT, title);
    DisableCursor();
    // Camera setup
    Camera3D camera = { 0 };
    camera.position = { 5.0f, 5.0f, 5.0f };
    camera.target = { 0.0f, 0.0f, 0.0f };
    camera.up = { 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    Model model = LoadModel("model4.glb");

    bool free_mode = false;

    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_S) || IsKeyDown(KEY_A) || IsKeyDown(KEY_D))
            free_mode = true;

        if (free_mode)
            UpdateCamera(&camera, CAMERA_FREE);
        else
            UpdateCamera(&camera, CAMERA_ORBITAL);

        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode3D(camera);
        

        DrawModel(model, { 0.0f, 0.0f, 0.0f }, 0.1f, WHITE);

        for (int i = -MAP_SIZE / 2; i < MAP_SIZE / 2; i++)
        {
            for (int j = -MAP_SIZE / 2; j < MAP_SIZE / 2; j++)
            {
                Color cellColor = ((i + j) % 2 == 0) ? LIGHTGRAY : GRAY;

                DrawCube({ i * CELL_SIZE, -0.05f, j * CELL_SIZE }, CELL_SIZE, 0.1f, CELL_SIZE, cellColor);

                DrawCubeWires({ i * CELL_SIZE, -0.05f, j * CELL_SIZE }, CELL_SIZE, 0.1f, CELL_SIZE, BLACK);
            }
        }

        EndMode3D();

        DrawText("Colored Grid Ground Example", 10, 10, 20, DARKGRAY);
        EndDrawing();
    }

    UnloadModel(model);
    CloseWindow();
}