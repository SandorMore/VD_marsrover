#ifndef ROVER_H
#define ROVER_H

#include <iostream>
#include <vector>
#include <queue>
#include <cmath>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <memory>

const int MAP_SIZE = 50;
const int MAX_BATTERY = 100;
const int DAY_DURATION = 16 * 2;
const int NIGHT_DURATION = 8 * 2;

const int K = 2;

struct LogEntry
{
    int timeStep;

    int x;
    int y;

    int battery;

    int speed;

    int distance;

    int mineralsCollected;

    std::string action;

    bool isDay;

    std::string timestamp;

    std::string toString() const;
};

struct Position
{
    int x;
    int y;

    Position(int x = 0, int y = 0);

    bool operator==(const Position& other) const;
};

struct PositionHash
{
    size_t operator()(const Position& p) const;
};

enum MineralType
{
    MINERAL_NONE,
    MINERAL_BLUE,
    MINERAL_YELLOW,
    MINERAL_GREEN
};

struct Cell
{
    MineralType mineral;

    bool isObstacle;

    bool isStart;

    Cell();
};


struct RoverState
{
    Position pos;

    int battery;

    int dayTimeRemaining;

    bool isDay;

    std::unordered_set<
        Position,
        PositionHash> collected;

    int totalMinerals;

    int timeElapsed;

    int totalDistance;

    std::vector<LogEntry> log;

    RoverState();

    std::string getTimeString() const;

    void addLogEntry(
        int speed,
        const std::string& action);

    std::string getStateId() const;
};

struct AStarNode
{
    RoverState state;

    int g;
    int h;
    int f;

    std::shared_ptr<AStarNode> parent;

    std::string action;

    AStarNode(
        const RoverState& s,
        int gCost,
        int hCost,
        std::shared_ptr<AStarNode> p = nullptr,
        const std::string& a = "");

    bool operator>(
        const AStarNode& other) const;
};

int chebyshevDistance(
    const Position& a,
    const Position& b);

int heuristic(
    const RoverState& state,
    int maxTime,
    const Position& startPos);

bool isWalkable(
    int x,
    int y,
    const std::vector<std::vector<Cell>>& map);

bool isMineral(
    int x,
    int y,
    const std::vector<std::vector<Cell>>& map);

int calculateMoveEnergy(
    int speed,
    bool isDay);

int calculateMineEnergy(
    bool isDay);

int calculateWaitEnergy(
    bool isDay);

void updateTime(
    RoverState& state);

bool readMap(
    const std::string& filename,
    std::vector<std::vector<Cell>>& map,
    Position& startPos);

std::pair<
    std::vector<LogEntry>,
    int>
    aStarSearch(
        int maxTime,
        const std::vector<std::vector<Cell>>& map,
        const Position& startPos);

void saveLogToFile(const std::vector<LogEntry>& log, const std::string& filename);

std::vector<Position> buildFastRoute(std::vector<std::vector<Cell>> map, Position start);
#endif