#include "rover.h"
#include <iostream>
#include <string>
#include <cstdlib>
#include <ctime>
#include <thread>
#include "misc.h"
#include <chrono>

#define WIDTH 1200
#define HEIGHT 800
#define TARGET_FPS 60
#define RETURN_TIME 5

using namespace std;

//rover_log_1771854750.csv

#define MAX_HOURS_TSET 100

void main_loop(const char* title);

int main(int argc, char* argv[]) {
    string map_file = R"(C:\Users\Sanyi\Desktop\verseny\VD_Marsrover\VD_Marsrover\asd.txt)";
    int max_hours = MAX_HOURS_TSET; //atoi(argv[2])
    int max_half_hours = max_hours * 2;

    std::vector<std::vector<Cell>>map;

    Position start_position;

    std::thread t(main_loop, "VD_Marsrover");
    std::thread k(readMap, std::ref(map_file), std::ref(map), std::ref(start_position));

    if (k.joinable()) {
        k.join();
    }

    if (t.joinable()) {
        t.join();
    }
    return 0;
}
const float CELL_SIZE = 1.0f;

void main_loop(const char* title)
{
    InitWindow(WIDTH, HEIGHT, title);
    DisableCursor();

    Vector3 model_position = { 0.f, 0.f, 0.f };
    Camera3D camera = { 0 };
    camera.position = { 1.5f, 1.5f, 1.5f };
    camera.target = { 0.0f, 0.0f, 0.0f };
    camera.up = { 0.0f, 1.f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    Model model = LoadModel("model4.glb");

    bool free_mode = false;

    // Proper chrono type
    std::chrono::high_resolution_clock::time_point last_move_time =
        std::chrono::high_resolution_clock::now();

    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        bool isMoving =
            IsKeyDown(KEY_W) ||
            IsKeyDown(KEY_S) ||
            IsKeyDown(KEY_A) ||
            IsKeyDown(KEY_D);

        if (isMoving)
        {
            free_mode = true;
            last_move_time = std::chrono::high_resolution_clock::now();
        }
        else
        {
            auto now = std::chrono::high_resolution_clock::now();
            float seconds =
                std::chrono::duration<float>(now - last_move_time).count();

            if (seconds >= 5.0f)
            {
                free_mode = false;
                camera.target = model_position;
            }
        }

        if (free_mode)
            UpdateCamera(&camera, CAMERA_FREE);
        else
            UpdateCamera(&camera, CAMERA_ORBITAL);

        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode3D(camera);

        DrawModel(model, model_position, 0.1f, WHITE);

        for (int i = -MAP_SIZE / 2; i < MAP_SIZE / 2; i++)
        {
            for (int j = -MAP_SIZE / 2; j < MAP_SIZE / 2; j++)
            {
                Color cell_color = BROWN;

                DrawCube({ i * CELL_SIZE, -0.05f, j * CELL_SIZE },
                    CELL_SIZE, 0.1f, CELL_SIZE, cell_color);

                DrawCubeWires({ i * CELL_SIZE, -0.05f, j * CELL_SIZE },
                    CELL_SIZE, 0.1f, CELL_SIZE, BLACK);
            }
        }

        EndMode3D();

        DrawText("Martian expedition", 10, 10, 20, GREEN);
        EndDrawing();
    }

    UnloadModel(model);
    CloseWindow();
}