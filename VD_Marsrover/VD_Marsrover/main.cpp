#include "renderer.h"
#include "rover.h"

#include <vector>
#include <string>
#include <iostream>
#include <thread>

int main()
{
    std::string map_file = "asd.txt";

    int max_half_hours = 200;

    std::vector<std::vector<Cell>> map(MAP_SIZE, std::vector<Cell>(MAP_SIZE));

    Position start_position;

    if (!readMap(map_file, map, start_position))
    {
        std::cout << "Map load failed\n";
        return 1;
    }

    auto route = buildFastRoute(map, start_position);

    main_loop("VD_Marsrover", map, start_position, route);

    return 0;
}