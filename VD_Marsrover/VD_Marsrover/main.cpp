#include "renderer.h"
#include "rover.h"

#include <vector>
#include <string>
#include <iostream>

int main()
{
    std::string map_file =
        R"(asd.txt)";

    int max_half_hours = 200;

    std::vector<std::vector<Cell>> map(
        MAP_SIZE,
        std::vector<Cell>(MAP_SIZE)
    );

    Position start_position;

    readMap(map_file, map, start_position);

    if (!readMap(map_file, map, start_position))
    {
        std::cout << "Map load failed\n";
        return 1;
    }

    auto result =
        aStarSearch(max_half_hours, map, start_position);

    main_loop("VD_Marsrover", map, start_position);

    return 0;
}