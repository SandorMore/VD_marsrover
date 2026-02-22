#include "rover.h"
#include <chrono>

using namespace std;

// LogEntry metódusok
string LogEntry::toString() const {
    stringstream ss;
    ss << timeStep << "," << x << "," << y << "," << battery << ","
        << speed << "," << distance << "," << mineralsCollected << ","
        << action << "," << (isDay ? "DAY" : "NIGHT") << "," << timestamp;
    return ss.str();
}

// Position metódusok
Position::Position(int x, int y) : x(x), y(y) {}

bool Position::operator==(const Position& other) const {
    return x == other.x && y == other.y;
}

// PositionHash metódus
size_t PositionHash::operator()(const Position& p) const {
    return p.x * 53 + p.y;
}

// Cell metódusok
Cell::Cell() : mineral(MINERAL_NONE), isObstacle(false), isStart(false) {}

// RoverState metódusok
RoverState::RoverState() : battery(MAX_BATTERY), isDay(true),
dayTimeRemaining(DAY_DURATION), totalMinerals(0),
timeElapsed(0), totalDistance(0) {
}

RoverState::RoverState(const RoverState& other) {
    pos = other.pos;
    battery = other.battery;
    dayTimeRemaining = other.dayTimeRemaining;
    isDay = other.isDay;
    collected = other.collected;
    totalMinerals = other.totalMinerals;
    timeElapsed = other.timeElapsed;
    totalDistance = other.totalDistance;
    log = other.log;
    path = other.path;
}

string RoverState::getTimeString() const {
    int totalHalfHours = timeElapsed;
    int hours = totalHalfHours / 2;
    int halfHours = totalHalfHours % 2;
    stringstream ss;
    ss << setw(2) << setfill('0') << hours << ":"
        << (halfHours == 0 ? "00" : "30");
    return ss.str();
}

void RoverState::addLogEntry(int speed, const string& action) {
    LogEntry entry;
    entry.timeStep = timeElapsed;
    entry.x = pos.x;
    entry.y = pos.y;
    entry.battery = battery;
    entry.speed = speed;
    entry.distance = totalDistance;
    entry.mineralsCollected = totalMinerals;
    entry.action = action;
    entry.isDay = isDay;
    entry.timestamp = getTimeString();
    log.push_back(entry);
}

string RoverState::getStateId() const {
    stringstream ss;
    ss << pos.x << "," << pos.y << "," << totalMinerals;
    return ss.str();
}

// AStarNode metódusok
AStarNode::AStarNode(const RoverState& s, int gCost, int hCost,
    shared_ptr<AStarNode> p, const string& a)
    : state(s), g(gCost), h(hCost), f(gCost + hCost), parent(p), action(a) {
}

bool AStarNode::operator>(const AStarNode& other) const {
    if (state.totalMinerals != other.state.totalMinerals)
        return state.totalMinerals < other.state.totalMinerals;
    int lastSpeedThis = 0, lastSpeedOther = 0;
    if (!state.log.empty() && state.log.back().action.find("MOVE") != string::npos) {
        string act = state.log.back().action;
        lastSpeedThis = stoi(act.substr(5));
    }
    if (!other.state.log.empty() && other.state.log.back().action.find("MOVE") != string::npos) {
        string act = other.state.log.back().action;
        lastSpeedOther = stoi(act.substr(5));
    }
    if (lastSpeedThis != lastSpeedOther)
        return lastSpeedThis < lastSpeedOther;

    if (state.timeElapsed != other.state.timeElapsed)
        return state.timeElapsed > other.state.timeElapsed;

    return false;
}

int manhattanDistance(const Position& a, const Position& b) {
    return abs(a.x - b.x) + abs(a.y - b.y);
}

int chebyshevDistance(const Position& a, const Position& b) {
    return max(abs(a.x - b.x), abs(a.y - b.y));
}

vector<Position> getAllMinerals(const vector<vector<Cell>>& map) {
    vector<Position> minerals;
    for (int i = 0; i < MAP_SIZE; i++) {
        for (int j = 0; j < MAP_SIZE; j++) {
            if (map[i][j].mineral != MINERAL_NONE) {
                minerals.push_back(Position(i, j));
            }
        }
    }
    return minerals;
}

vector<Position> allMinerals;

int heuristic(const RoverState& state, int maxTime, const Position& startPos) {
    int timeLeft = maxTime - state.timeElapsed;
    if (timeLeft <= 0) return -1000;

    int remainingCount = allMinerals.size() - state.totalMinerals;

    if (remainingCount == 0) {
        int distToStart = chebyshevDistance(state.pos, startPos);
        if (distToStart <= timeLeft) {

            return 100;
        }
        else {
            return -500;
        }
    }

    int minDist = INT_MAX;
    for (const auto& m : allMinerals) {
        if (state.collected.find(m) == state.collected.end()) {
            int dist = chebyshevDistance(state.pos, m);
            minDist = min(minDist, dist);
        }
    }

    int speedBonus = 0;
    if (!state.log.empty() && state.log.back().action.find("MOVE") != string::npos) {
        string act = state.log.back().action;
        int lastSpeed = stoi(act.substr(5));
        speedBonus = lastSpeed * 2;
    }

    int energyBonus = 0;
    if (state.battery > 70) energyBonus = 5;
    else if (state.battery < 20) energyBonus = -10;

    int dayBonus = state.isDay ? 3 : -3;

    return remainingCount * 100 - minDist + speedBonus + energyBonus + dayBonus;
}

bool isWalkable(int x, int y, const vector<vector<Cell>>& map) {
    if (x < 0 || x >= MAP_SIZE || y < 0 || y >= MAP_SIZE) {
        return false;
    }
    if (map[x][y].isObstacle) {
        return false;
    }
    return true;
}

bool isMineral(int x, int y, const vector<vector<Cell>>& map) {
    if (x < 0 || x >= MAP_SIZE || y < 0 || y >= MAP_SIZE) return false;
    return map[x][y].mineral != MINERAL_NONE;
}

int calculateMoveEnergy(int speed, bool isDay) {
    int energy = K * speed * speed;
    if (isDay) energy -= 10;
    return energy;
}

int calculateMineEnergy(bool isDay) {
    int energy = 2;
    if (isDay) energy -= 10;
    return energy;
}

int calculateWaitEnergy(bool isDay) {
    int energy = 1;
    if (isDay) energy -= 10;
    return energy;
}

void updateTime(RoverState& state) {
    state.timeElapsed++;
    state.dayTimeRemaining--;

    if (state.dayTimeRemaining == 0) {
        state.isDay = !state.isDay;
        state.dayTimeRemaining = state.isDay ? DAY_DURATION : NIGHT_DURATION;
    }
}

bool readMap(const string& filename, vector<vector<Cell>>& map, Position& startPos) {
    ifstream mapFile(filename);
    if (!mapFile.is_open()) {
        cerr << "Error: Cannot open map file: " << filename << endl;
        return false;
    }

    string line;
    int sor = 0;

    while (getline(mapFile, line) && sor < MAP_SIZE) {
        stringstream ss(line);
        string token;
        int oszlop = 0;

        while (getline(ss, token, ',') && oszlop < MAP_SIZE) {
            token.erase(0, token.find_first_not_of(" \t"));
            token.erase(token.find_last_not_of(" \t") + 1);

            if (token.empty()) {
                // Üres token = '.' 
                oszlop++;
                continue;
            }

            char c = token[0];

            switch (c) {
            case '#':
                map[sor][oszlop].isObstacle = true;
                break;
            case 'B':
                map[sor][oszlop].mineral = MINERAL_BLUE;
                break;
            case 'Y':
                map[sor][oszlop].mineral = MINERAL_YELLOW;
                break;
            case 'G':
                map[sor][oszlop].mineral = MINERAL_GREEN;
                break;
            case 'S':
                map[sor][oszlop].isStart = true;
                startPos = Position(sor, oszlop);
                break;
            default:
                break;
            }
            oszlop++;
        }
        sor++;
    }

    mapFile.close();

    allMinerals = getAllMinerals(map);
    cout << "Total minerals on map: " << allMinerals.size() << endl;

    cout << "\nTerkep ellenorzes (elso 5 sor):" << endl;
    for (int i = 0; i < 5 && i < MAP_SIZE; i++) {
        for (int j = 0; j < 30 && j < MAP_SIZE; j++) {
            if (map[i][j].isObstacle) cout << "#";
            else if (map[i][j].mineral != MINERAL_NONE) {
                if (map[i][j].mineral == MINERAL_BLUE) cout << "B";
                else if (map[i][j].mineral == MINERAL_YELLOW) cout << "Y";
                else if (map[i][j].mineral == MINERAL_GREEN) cout << "G";
            }
            else if (map[i][j].isStart) cout << "S";
            else cout << ".";
        }
        cout << "  <- " << i << ". sor" << endl;
    }
    cout << endl;

    return true;
}

const int dx[] = { -1, -1, -1, 0, 0, 1, 1, 1 };
const int dy[] = { -1, 0, 1, -1, 1, -1, 0, 1 };

pair<vector<LogEntry>, int> aStarSearch(int maxTime, const vector<vector<Cell>>& map, const Position& startPos) {
    auto startTime = chrono::steady_clock::now();

    priority_queue<AStarNode, vector<AStarNode>, greater<AStarNode>> openSet;
    unordered_map<string, int> bestG;

    RoverState startState;
    startState.pos = startPos;
    startState.path.push_back(startPos);
    startState.addLogEntry(0, "START");

    int hStart = heuristic(startState, maxTime, startPos);
    openSet.push(AStarNode(startState, 0, hStart));
    bestG[startState.getStateId()] = 0;

    int maxMinerals = 0;
    RoverState bestState;
    int bestTimeAtStart = INT_MAX;

    int iterations = 0;
    const int MAX_ITERATIONS = 5000000;

    while (!openSet.empty() && iterations < MAX_ITERATIONS) {
        iterations++;

        auto currentTime = chrono::steady_clock::now();
        auto elapsed = chrono::duration_cast<chrono::seconds>(currentTime - startTime).count();
        if (elapsed > 120) {
            cout << "Search timeout after 60 seconds, stopping..." << endl;
            break;
        }

        AStarNode current = openSet.top();
        openSet.pop();

        string stateId = current.state.getStateId();
        if (bestG.find(stateId) != bestG.end() && bestG[stateId] < current.g) {
            continue;
        }

        if (current.state.pos == startPos) {
            if (current.state.totalMinerals > maxMinerals) {
                maxMinerals = current.state.totalMinerals;
                bestState = current.state;
                bestTimeAtStart = current.state.timeElapsed;
                cout << "Found solution: " << maxMinerals << " minerals, returned to start at time "
                    << current.state.getTimeString() << endl;
            }
            else if (current.state.totalMinerals == maxMinerals &&
                current.state.timeElapsed < bestTimeAtStart) {
                bestState = current.state;
                bestTimeAtStart = current.state.timeElapsed;
                cout << "Better time for " << maxMinerals << " minerals: "
                    << current.state.getTimeString() << endl;
            }
        }

        if (current.state.totalMinerals == allMinerals.size() && current.state.pos == startPos) {
            bestState = current.state;
            cout << "Optimal solution found! Collected all minerals and returned to start." << endl;
            break;
        }

        if (current.state.timeElapsed >= maxTime) {
            continue;
        }

        for (int i = 0; i < 8; i++) {
            int newX = current.state.pos.x + dx[i];
            int newY = current.state.pos.y + dy[i];

            if (!isWalkable(newX, newY, map)) continue;

            for (int speed = 1; speed <= 3; speed++) {
                RoverState newState = current.state;
                newState.pos = Position(newX, newY);
                newState.totalDistance++;

                int energyChange = calculateMoveEnergy(speed, current.state.isDay);
                newState.battery = max(0, min(MAX_BATTERY,
                    current.state.battery - energyChange));

                if (newState.battery <= 0 && !newState.isDay) continue;

                updateTime(newState);

                string action = "MOVE_" + to_string(speed);
                newState.addLogEntry(speed, action);
                newState.path.push_back(newState.pos);

                string newStateId = newState.getStateId();
                int newG = newState.timeElapsed;

                if (bestG.find(newStateId) == bestG.end() || newG < bestG[newStateId]) {
                    bestG[newStateId] = newG;
                    int h = heuristic(newState, maxTime, startPos);
                    openSet.push(AStarNode(newState, newG, h,
                        make_shared<AStarNode>(current), action));
                }
            }
        }

        if (isMineral(current.state.pos.x, current.state.pos.y, map) &&
            current.state.collected.find(current.state.pos) == current.state.collected.end()) {

            RoverState newState = current.state;

            int energyChange = calculateMineEnergy(current.state.isDay);
            newState.battery = max(0, min(MAX_BATTERY,
                current.state.battery - energyChange));

            newState.collected.insert(current.state.pos);
            newState.totalMinerals++;

            updateTime(newState);

            if (newState.battery <= 0 && !newState.isDay) {
                continue;
            }

            newState.addLogEntry(0, "MINE");
            newState.path.push_back(newState.pos);

            string newStateId = newState.getStateId();
            int newG = newState.timeElapsed;

            if (bestG.find(newStateId) == bestG.end() || newG < bestG[newStateId]) {
                bestG[newStateId] = newG;
                int h = heuristic(newState, maxTime, startPos);
                openSet.push(AStarNode(newState, newG, h,
                    make_shared<AStarNode>(current), "MINE"));
            }
        }

        if (!current.state.isDay && current.state.battery < 30) {
            RoverState newState = current.state;

            int energyChange = calculateWaitEnergy(current.state.isDay);
            newState.battery = max(0, min(MAX_BATTERY,
                current.state.battery - energyChange));

            updateTime(newState);

            newState.addLogEntry(0, "WAIT");
            newState.path.push_back(newState.pos);

            string newStateId = newState.getStateId();
            int newG = newState.timeElapsed;

            if (bestG.find(newStateId) == bestG.end() || newG < bestG[newStateId]) {
                bestG[newStateId] = newG;
                int h = heuristic(newState, maxTime, startPos);
                openSet.push(AStarNode(newState, newG, h,
                    make_shared<AStarNode>(current), "WAIT"));
            }
        }
    }

    auto endTime = chrono::steady_clock::now();
    auto elapsed = chrono::duration_cast<chrono::milliseconds>(endTime - startTime).count();
    cout << "Search completed in " << elapsed << " ms" << endl;
    cout << "Iterations: " << iterations << endl;

    if (maxMinerals > 0) {
        return make_pair(bestState.log, maxMinerals);
    }

    return make_pair(vector<LogEntry>(), 0);
}

void saveLogToFile(const vector<LogEntry>& log, const string& filename) {
    ofstream file(filename);
    if (!file.is_open()) {
        cerr << "Error: Cannot create log file: " << filename << endl;
        return;
    }

    file << "TimeStep,X,Y,Battery,Speed,Distance,Minerals,Action,TimeOfDay,Timestamp\n";

    for (const auto& entry : log) {
        file << entry.toString() << "\n";
    }

    file.close();
    cout << "Log saved to: " << filename << endl;
}