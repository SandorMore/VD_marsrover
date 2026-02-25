#include "rover.h"

#include <queue>
#include <unordered_map>
#include <vector>
#include <algorithm>



struct PathNode
{
    Position pos;
    int g;
    int h;
    int f;
};



int simpleHeuristic(const Position& a, const Position& b)
{
    return std::max(
        abs(a.x - b.x),
        abs(a.y - b.y)
    );
}



bool findPathToTarget(
    const Position& start,
    const Position& target,
    const std::vector<std::vector<Cell>>& map,
    std::vector<Position>& outPath
)
{
    auto cmp = [](const std::pair<int, Position>& a,
        const std::pair<int, Position>& b) {
            return a.first > b.first; 
        };

    std::priority_queue<
        std::pair<int, Position>,
        std::vector<std::pair<int, Position>>,
        decltype(cmp)
    > open(cmp);

    std::unordered_map<Position, Position, PositionHash> parent;

    std::unordered_map<Position, int, PositionHash> cost;

    open.push({ 0, start });

    cost[start] = 0;

    while (!open.empty())
    {
        Position current = open.top().second;
        open.pop();

        if (current == target)
        {
            outPath.clear();

            Position p = target;

            while (!(p == start))
            {
                outPath.push_back(p);
                p = parent[p];
            }

            std::reverse(
                outPath.begin(),
                outPath.end()
            );

            return true;
        }


        for (int dx = -1; dx <= 1; dx++)
            for (int dy = -1; dy <= 1; dy++)
            {
                if (dx == 0 && dy == 0)
                    continue;

                Position next(
                    current.x + dx,
                    current.y + dy
                );

                if (!isWalkable(next.x, next.y, map))
                    continue;

                int newCost =
                    cost[current] + 1;

                if (!cost.count(next) ||
                    newCost < cost[next])
                {
                    cost[next] = newCost;

                    int priority =
                        newCost +
                        simpleHeuristic(next, target);

                    open.push({ priority, next });

                    parent[next] = current;
                }
            }
    }

    return false;
}



Position findNearestMineral(
    const Position& rover,
    const std::vector<std::vector<Cell>>& map
)
{
    int bestDist = INT_MAX;

    Position best = rover;

    for (int x = 0; x < MAP_SIZE; x++)
        for (int y = 0; y < MAP_SIZE; y++)
        {
            if (map[x][y].mineral != MINERAL_NONE)
            {
                Position p(x, y);

                int d =
                    simpleHeuristic(rover, p);

                if (d < bestDist)
                {
                    bestDist = d;
                    best = p;
                }
            }
        }

    return best;
}



std::vector<Position> buildFastRoute(
    std::vector<std::vector<Cell>> map,
    Position start
)
{
    std::vector<Position> route;

    Position rover = start;

    while (true)
    {
        Position target =
            findNearestMineral(rover, map);

        if (target == rover)
            break;

        std::vector<Position> path;

        if (!findPathToTarget(
            rover,
            target,
            map,
            path))
            break;


        for (Position p : path)
        {
            route.push_back(p);
        }


        map[target.x][target.y].mineral =
            MINERAL_NONE;

        rover = target;
    }

    return route;
}