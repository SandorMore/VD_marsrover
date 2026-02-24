#include "renderer.h"
#include "transforms.h"

#define WIDTH 1200
#define HEIGHT 800
#define TARGET_FPS 60
#define RETURN_TIME 5

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

            if (map[x][y].mineral != MINERAL_NONE)
                cell_color = BLUE;

            if (map[x][y].isStart)
                cell_color = GREEN;

            DrawCube(pos, CELL_SIZE, 0.1f, CELL_SIZE, cell_color);
            DrawCubeWires(pos, CELL_SIZE, 0.1f, CELL_SIZE, BLACK);
        }
    }
}

void main_loop(const char* title,
    const std::vector<std::vector<Cell>>& map,
    const Position& startPos)
{
    InitWindow(WIDTH, HEIGHT, title);
    DisableCursor();

    Camera3D camera = { 0 };

    camera.position = { 50.f, 50.f, 50.f };
    camera.target = { 0.f,0.f,0.f };
    camera.up = { 0.f,1.f,0.f };
    camera.fovy = 45.f;
    camera.projection = CAMERA_PERSPECTIVE;

    Model roverModel = LoadModel("model4.glb");

    Vector3 roverPos = gridToWorld(startPos);

    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        UpdateCamera(&camera, CAMERA_ORBITAL);

        BeginDrawing();

        ClearBackground(RAYWHITE);

        BeginMode3D(camera);

        drawMap(map);

        DrawModel(roverModel, roverPos, 0.1f, WHITE);

        EndMode3D();

        DrawText("Mars Rover", 10, 10, 20, GREEN);

        EndDrawing();
    }

    UnloadModel(roverModel);
    CloseWindow();
}