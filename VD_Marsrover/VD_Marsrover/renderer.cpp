#include "renderer.h"
#include "transforms.h"
#include "rover.h"

#include "raylib.h"
#include <cmath>

#define WIDTH 1200
#define HEIGHT 800
#define TARGET_FPS 60
#define MOVE_DELAY 30

void drawMap(const std::vector<std::vector<Cell>>& map)
{
    for (int x = 0; x < MAP_SIZE; x++)
    {
        for (int y = 0; y < MAP_SIZE; y++)
        {
            Vector3 pos = gridToWorld(Position(x, y));
            pos.y = -0.05f;

            Color cell_color = BROWN;

            if (map[x][y].isObstacle)
                cell_color = DARKGRAY;
            else if (map[x][y].mineral != MINERAL_NONE)
                cell_color = BLUE;
            else if (map[x][y].isStart)
                cell_color = GREEN;

            DrawCube(pos, CELL_SIZE, 0.1f, CELL_SIZE, cell_color);
            DrawCubeWires(pos, CELL_SIZE, 0.1f, CELL_SIZE, BLACK);
        }
    }
}

void main_loop(
    const char* title,
    const std::vector<std::vector<Cell>>& map,
    const Position& startPos,
    const std::vector<Position>& route
)
{
    InitWindow(WIDTH, HEIGHT, title);

    if (!IsWindowReady())
    {
        printf("Window failed!\n");
        return;
    }

    DisableCursor();

    Camera3D camera = { 0 };
    camera.position = { 60.f, 60.f, 60.f };
    camera.target = gridToWorld(startPos);
    camera.up = { 0.f, 1.f, 0.f };
    camera.fovy = 45.f;
    camera.projection = CAMERA_PERSPECTIVE;

    Model roverModel = LoadModel("model4.glb");

    int routeIndex = 0;
    int frameCounter = 0;

    Vector3 roverPos = gridToWorld(startPos);

    SetTargetFPS(TARGET_FPS);

    while (!WindowShouldClose())
    {
        frameCounter++;
        if (frameCounter >= MOVE_DELAY && routeIndex < route.size())
        {
            roverPos = gridToWorld(route[routeIndex]);
            routeIndex++;
            frameCounter = 0;
        }

        Vector3 targetPos = roverPos;
        camera.target.x += (targetPos.x - camera.target.x) * 0.1f;
        camera.target.y += (targetPos.y - camera.target.y) * 0.1f;
        camera.target.z += (targetPos.z - camera.target.z) * 0.1f;

        float radius = 15.f;
        float angle = GetTime() * 0.5f;
        camera.position.x = camera.target.x + radius * cosf(angle);
        camera.position.z = camera.target.z + radius * sinf(angle);
        camera.position.y = camera.target.y + 3.f;

        UpdateCamera(&camera, CAMERA_ORBITAL);

        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode3D(camera);

        drawMap(map);

        DrawModel(roverModel, roverPos, 0.1f, WHITE);

        EndMode3D();

        DrawText("Mars Rover", 10, 10, 20, GREEN);
        DrawFPS(10, 40);

        EndDrawing();
    }

    UnloadModel(roverModel);
    CloseWindow();
}