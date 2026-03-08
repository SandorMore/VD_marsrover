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
    return std::max(abs(a.x - b.x), abs(a.y - b.y));
}

bool findPathToTarget(const Position& start, const Position& target, const std::vector<std::vector<Cell>>& map, std::vector<Position>& outPath)
{
    auto cmp = [](const std::pair<int, Position>& a, const std::pair<int, Position>& b) {return a.first > b.first;};

    std::priority_queue< std::pair<int, Position>, std::vector<std::pair<int, Position>>, decltype(cmp) > open(cmp);

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



Position findNearestMineral(const Position& rover, const std::vector<std::vector<Cell>>& map)
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



std::vector<Position> buildFastRoute(std::vector<std::vector<Cell>> map, Position start)
{
    // Route starts at the rover's initial position.
    std::vector<Position> route;
    route.push_back(start);

    Position rover = start;

    // Time and battery model according to the task description.
    int battery = MAX_BATTERY;
    int timeStep = 0;                      // half-hour units
    bool isDay = true;
    int phaseRemaining = DAY_DURATION;     // remaining half-hours in current day/night phase

    auto advanceHalfHour = [&](int energyUse) -> bool {
        // Charging during the day
        int charge = isDay ? 10 : 0;
        battery += charge - energyUse;
        if (battery > MAX_BATTERY) battery = MAX_BATTERY;
        if (battery <= 0) return false;

        timeStep++;
        phaseRemaining--;

        if (phaseRemaining == 0) {
            isDay = !isDay;
            phaseRemaining = isDay ? DAY_DURATION : NIGHT_DURATION;
        }

        // Hard limit: 24 hours = 48 half-hour steps
        if (timeStep >= 24 * 2) return false;
        return true;
    };

    auto chooseSpeed = [&](int /*distToTarget*/) -> int {
        // Simple heuristic: faster when battery is high and it's day.
        if (isDay && battery > 70) return 3;
        if (battery > 40) return 2;
        return 1;
    };

    int collectedMinerals = 0;

    while (true)
    {
        Position target = findNearestMineral(rover, map);

        if (target == rover)
            break;

        std::vector<Position> path;

        if (!findPathToTarget(
            rover,
            target,
            map,
            path))
            break;

        // Walk along the path step-by-step, respecting battery and 24h limit.
        for (Position p : path)
        {
            int speed = chooseSpeed(simpleHeuristic(rover, target));
            int moveEnergy = K * speed * speed;
            if (!advanceHalfHour(moveEnergy)) {
                // Out of time or battery; stop planning further.
                return route;
            }

            rover = p;
            route.push_back(rover);
        }

        // We reached a mineral; simulate mining for one half-hour.
        if (map[rover.x][rover.y].mineral != MINERAL_NONE) {
            int mineEnergy = 2; // per spec
            if (!advanceHalfHour(mineEnergy)) {
                return route;
            }

            map[rover.x][rover.y].mineral = MINERAL_NONE;
            collectedMinerals++;
        }

        // If we're close to the time limit or battery is low, head back to start.
        if (timeStep >= 24 * 2 - 8 || battery < 20) {
            break;
        }
    }

    // Try to return to start within remaining resources.
    if (!(rover == start)) {
        std::vector<Position> backPath;
        if (findPathToTarget(rover, start, map, backPath)) {
            for (Position p : backPath) {
                int speed = chooseSpeed(simpleHeuristic(rover, start));
                int moveEnergy = K * speed * speed;
                if (!advanceHalfHour(moveEnergy)) {
                    return route;
                }
                rover = p;
                route.push_back(rover);
            }
        }
    }

    return route;
}