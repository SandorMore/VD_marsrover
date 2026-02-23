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


void CoreLoop(const char* title) {
    
    InitWindow(WIDTH, HEIGHT, title);
    
    unsigned ch_to_draw = 10;
    
    Vector3 position{1, 1, 1};


    Camera3D camera = { 0 };
    camera.position = { 5.0f, 5.0f, 5.0f };
    camera.target = { 0.0f, 1.0f, 0.0f };
    camera.up = { 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    Model model = LoadModel("model4.glb");
    
    while (!WindowShouldClose()) {
        UpdateCamera(&camera, CAMERA_ORBITAL);

        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode3D(camera);

        DrawModel(model, { 0.0f, 0.0f, 0.0f }, 1.0f, WHITE);
        DrawGrid(20, 1.0f);

        EndMode3D();
        EndDrawing();
    }
    UnloadModel(model);
    CloseWindow();
}