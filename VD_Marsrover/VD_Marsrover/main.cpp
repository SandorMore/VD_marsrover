
#include <iostream>
#include <string>
#include <cstdlib>
#include <ctime>
#include <thread>
#include <vector>
#include <cstdint>
#include <cstdio>

#include "misc.h"
#include "raymath.h"
#include "heightmap.h"
#include "rover.h"
#include "mesh_collisions.h"


#define WIDTH 1200
#define HEIGHT 800
#define TARGET_FPS 60

using namespace std;

//rover_log_1771854750.csv

#define MAX_HOURS_TSET 100

Vector3 normal;
Vector3 roverPosPrev;

void DrawRover(Model model,Mesh ground_mesh,Matrix model_matrix, Vector3 roverPos) {
   
    float hCenter = GetHeightAt(ground_mesh, model_matrix, roverPos);

    float hFront = GetHeightAt(ground_mesh, model_matrix, roverPos + Vector3{ 0.2f, 0, 0 });
    float hBack = GetHeightAt(ground_mesh, model_matrix, roverPos + Vector3{ -0.2f, 0, 0 });

    float hRight = GetHeightAt(ground_mesh, model_matrix, roverPos + Vector3{ 0, 0, 0.2f });
    float hLeft = GetHeightAt(ground_mesh, model_matrix, roverPos + Vector3{ 0, 0,-0.2f });

    roverPos.y = hCenter;

    Vector3 new_normal;

    new_normal.x = hBack - hFront;
    new_normal.y = 0.4f;
    new_normal.z = hLeft - hRight;

    new_normal = Vector3Normalize(new_normal);

	normal = new_normal;
	roverPosPrev = roverPos;

    
}


void CoreLoop(const char* title);

int main(int argc, char* argv[]) {
    string mapFile = R"(C:\Users\Sanyi\Desktop\verseny\VD_Marsrover\VD_Marsrover\asd.txt)";
    int maxHours = MAX_HOURS_TSET;
    int maxHalfHours = maxHours * 2;

    std::thread t();
    CoreLoop("Marsrover");
}
const float CELL_SIZE = 1.0f;

void CoreLoop(const char* title)
{
    InitWindow(WIDTH, HEIGHT, title);
    DisableCursor();
    Camera3D camera = { 0 };
    camera.position = { 5.0f, 5.0f, 5.0f };
    camera.target = { 0.0f, 0.0f, 0.0f };
    camera.up = { 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    Model model = LoadModel("model4.glb");

    bool free_mode = false;

    SetTargetFPS(60);



    Image heightmap = GenerateHeightmapImage(
        500,500,
        0.05f,0.09f,    
        5850.0f  
    );

    Mesh ground_mesh = GenMeshHeightmap(heightmap,{ 50, 10, 50 });

    

    Texture2D tex = LoadTexture("u.png");
    Material ground_material = LoadMaterialDefault();
    ground_material.maps[MATERIAL_MAP_ALBEDO].texture = tex;
    SetMaterialTexture(&ground_material, MATERIAL_MAP_ALBEDO, tex);

    Vector3 centerPos = { -25.5, 0, -25.5 }; 
    Matrix model_matrix = MatrixTranslate(centerPos.x, centerPos.y, centerPos.z);
    DrawMesh(ground_mesh, ground_material, model_matrix);
    
	DrawRover(model, ground_mesh, model_matrix, { 0,0,0 });
    Vector3 ize = Vector3CrossProduct({ 0,1,0 }, normal);
    float ize2 = (RAD2DEG * acos(Vector3DotProduct({ 0,1,0 }, normal)));
    float ize3 = GetHeightAt(ground_mesh, model_matrix, { 0,0,0 });

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
        


        DrawMesh(ground_mesh, ground_material, model_matrix);
        

        DrawModelEx(
            model,
            {0,ize3,0},
            ize,ize2,
            { 0.1f,0.1f,0.1f },
            WHITE
        );

        for (int i = -MAP_SIZE / 2; i < MAP_SIZE / 2; i++)
        {
            for (int j = -MAP_SIZE / 2; j < MAP_SIZE / 2; j++)
            {
                Color cellColor = ((i + j) % 2 == 0) ? LIGHTGRAY : GRAY;

               DrawCubeWires({ i * CELL_SIZE, -0.05f, j * CELL_SIZE }, CELL_SIZE, 0.1f, CELL_SIZE, BLACK);
            }
        }
        
        

        EndMode3D();

        DrawText("Colored Grid Ground Example", 10, 10, 20, DARKGRAY);
        EndDrawing();
    }

    UnloadModel(model);
    UnloadTexture(tex);
    CloseWindow();
}

