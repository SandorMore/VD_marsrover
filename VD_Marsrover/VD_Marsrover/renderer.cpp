#include "renderer.h"
#include "transforms.h"
#include "rover.h"

#include "raylib.h"
#include <cmath>

#define WIDTH 1200
#define HEIGHT 800
#define TARGET_FPS 60
#define MOVE_DELAY 20

Color GetSkyColor(float time)
{
    Color day = { 255, 240, 170, 255 };
    Color sunset = { 220, 90, 40, 255 };
    Color night = { 15, 10, 25, 255 };

    Color result;

    if (time < 0.5f)
    {
        float t = time * 2.0f;

        result.r = day.r + (sunset.r - day.r) * t;
        result.g = day.g + (sunset.g - day.g) * t;
        result.b = day.b + (sunset.b - day.b) * t;
        result.a = 255;
    }
    else
    {
        float t = (time - 0.5f) * 2.0f;

        result.r = sunset.r + (night.r - sunset.r) * t;
        result.g = sunset.g + (night.g - sunset.g) * t;
        result.b = sunset.b + (night.b - sunset.b) * t;
        result.a = 255;
    }

    return result;
}

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

void main_loop(const char* title, const std::vector<std::vector<Cell>>& map, const Position& startPos, const std::vector<Position>& route)
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
        float dayTime = fmod(GetTime() * 0.02f, 1.0f);
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
        camera.position.y = camera.target.y + 8.f;

        UpdateCamera(&camera, CAMERA_ORBITAL);

        BeginDrawing();
        ClearBackground(GetSkyColor(dayTime));

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