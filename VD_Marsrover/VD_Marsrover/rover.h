#ifndef ROVER_H
#define ROVER_H

#include <iostream>
#include <vector>
#include <queue>
#include <cmath>
#include <unordered_set>
#include <unordered_map>
#include <functional>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <climits>
#include <memory>

using namespace std;

// Constants
const int MAP_SIZE = 50;
const int MAX_BATTERY = 100;
const int DAY_DURATION = 16 * 2;
const int NIGHT_DURATION = 8 * 2;
const int K = 2;

struct LogEntry {
    int timeStep;
    int x, y;
    int battery;
    int speed;
    int distance;
    int mineralsCollected;
    string action;
    bool isDay;
    string timestamp;

    string toString() const;
};

struct Position {
    int x, y;
    Position(int x = 0, int y = 0);
    bool operator==(const Position& other) const;
};

struct PositionHash {
    size_t operator()(const Position& p) const;
};


enum MineralType {
    MINERAL_NONE,
    MINERAL_BLUE,
    MINERAL_YELLOW,
    MINERAL_GREEN
};

struct Cell {
    MineralType mineral;
    bool isObstacle;
    bool isStart;

    Cell();
};

struct RoverState {
    Position pos;
    int battery;
    int dayTimeRemaining;
    bool isDay;
    unordered_set<Position, PositionHash> collected;
    int totalMinerals;
    int timeElapsed;
    int totalDistance;
    vector<LogEntry> log;
    vector<Position> path;

    RoverState();
    RoverState(const RoverState& other);
    string getTimeString() const;
    void addLogEntry(int speed, const string& action);
    string getStateId() const;
};

struct AStarNode {
    RoverState state;
    int g;
    int h;
    int f;
    shared_ptr<AStarNode> parent;
    string action;

    AStarNode(const RoverState& s, int gCost, int hCost,
        shared_ptr<AStarNode> p = nullptr, const string& a = "");
    bool operator>(const AStarNode& other) const;
};

int chebyshevDistance(const Position& a, const Position& b);
vector<Position> getRemainingMinerals(const unordered_set<Position, PositionHash>& collected, const vector<vector<Cell>>& map);
int heuristic(const RoverState& state, int maxTime, const Position& startPos, const vector<vector<Cell>>& map);
bool isWalkable(int x, int y, const vector<vector<Cell>>& map);
bool isMineral(int x, int y, const vector<vector<Cell>>& map);
int calculateMoveEnergy(int speed, bool isDay);
int calculateMineEnergy(bool isDay);
int calculateWaitEnergy(bool isDay);
void updateTime(RoverState& state);
bool readMap(const string& filename, vector<vector<Cell>>& map, Position& startPos);
pair<vector<LogEntry>, int> aStarSearch(int maxTime, const vector<vector<Cell>>& map, const Position& startPos);
void saveLogToFile(const vector<LogEntry>& log, const string& filename);

#endif