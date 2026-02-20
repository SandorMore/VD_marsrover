#include <iostream>
#include <cstdlib>
#include <vector>
#include <cmath>
#include "raylib.h"
#include "raymath.h"

#define WIDTH 800
#define HEIGHT 600

int main(int d)
{
	InitWindow(WIDTH, HEIGHT, "Hello Raylib");
	while (!WindowShouldClose()) {
		BeginDrawing();
		ClearBackground(RAYWHITE);
		DrawText("Hello Raylib!", 190, 200, 20, LIGHTGRAY);
		EndDrawing();
	}
	CloseWindow();
	std::cout << "Done\n";
}
