#include "rover.h"
#include <chrono>

using namespace std;

vector<Position> allMinerals;

string LogEntry::toString() const
{
    stringstream ss;

    ss << timeStep << ","
        << x << ","
        << y << ","
        << battery << ","
        << speed << ","
        << distance << ","
        << mineralsCollected << ","
        << action << ","
        << (isDay ? "DAY" : "NIGHT") << ","
        << timestamp;

    return ss.str();
}

Position::Position(int x, int y)
    : x(x), y(y)
{
}

bool Position::operator==(const Position& other) const
{
    return x == other.x && y == other.y;
}

size_t PositionHash::operator()(const Position& p) const
{
    return p.x * 53 + p.y;
}

Cell::Cell()
    : mineral(MINERAL_NONE),
    isObstacle(false),
    isStart(false)
{
}

RoverState::RoverState()
    : battery(MAX_BATTERY),
    isDay(true),
    dayTimeRemaining(DAY_DURATION),
    totalMinerals(0),
    timeElapsed(0),
    totalDistance(0)
{
}

string RoverState::getTimeString() const
{
    int hours = timeElapsed / 2;
    int half = timeElapsed % 2;

    stringstream ss;

    ss << setw(2) << setfill('0') << hours
        << ":" << (half ? "30" : "00");

    return ss.str();
}

void RoverState::addLogEntry(int speed, const string& action)
{
    LogEntry e;

    e.timeStep = timeElapsed;
    e.x = pos.x;
    e.y = pos.y;
    e.battery = battery;
    e.speed = speed;
    e.distance = totalDistance;
    e.mineralsCollected = totalMinerals;
    e.action = action;
    e.isDay = isDay;
    e.timestamp = getTimeString();

    log.push_back(e);
}


string RoverState::getStateId() const
{
    stringstream ss;

    ss << pos.x << ","
        << pos.y << ","
        << battery << ","
        << isDay << ","
        << dayTimeRemaining << ",";

    for (const auto& p : collected)
    {
        ss << p.x << ":" << p.y << ";";
    }

    return ss.str();
}

AStarNode::AStarNode(
    const RoverState& s,
    int gCost,
    int hCost,
    shared_ptr<AStarNode> p,
    const string& a)
    : state(s),
    g(gCost),
    h(hCost),
    f(gCost + hCost),
    parent(p),
    action(a)
{
}


bool AStarNode::operator>(const AStarNode& other) const
{
    if (f != other.f)
        return f > other.f;

    if (state.totalMinerals != other.state.totalMinerals)
        return state.totalMinerals < other.state.totalMinerals;

    return g > other.g;
}

int chebyshevDistance(const Position& a, const Position& b)
{
    return max(abs(a.x - b.x), abs(a.y - b.y));
}


vector<Position> getAllMinerals(const vector<vector<Cell>>& map)
{
    vector<Position> v;

    for (int i = 0; i < MAP_SIZE; i++)
        for (int j = 0; j < MAP_SIZE; j++)
            if (map[i][j].mineral != MINERAL_NONE)
                v.push_back(Position(i, j));

    return v;
}

int heuristic(
    const RoverState& s,
    int maxTime,
    const Position& start)
{
    int timeLeft = maxTime - s.timeElapsed;

    if (timeLeft <= 0)
        return 1000000;

    int remaining = allMinerals.size() - s.totalMinerals;

    if (remaining == 0)
    {
        int d = chebyshevDistance(s.pos, start);
        return d * 2;
    }

    int best = 999999;

    for (const auto& m : allMinerals)
    {
        if (s.collected.find(m) == s.collected.end())
        {
            int d = chebyshevDistance(s.pos, m);
            best = min(best, d);
        }
    }

    return best + remaining * 5;
}


int calculateMoveEnergy(int speed, bool isDay)
{
    int e = K * speed * speed;

    if (isDay)
        e -= 10;

    return e;
}

int calculateMineEnergy(bool isDay)
{
    int e = 2;

    if (isDay)
        e -= 10;

    return e;
}

int calculateWaitEnergy(bool isDay)
{
    int e = 1;

    if (isDay)
        e -= 10;

    return e;
}

void updateTime(RoverState& s)
{
    s.timeElapsed++;
    s.dayTimeRemaining--;

    if (s.dayTimeRemaining == 0)
    {
        s.isDay = !s.isDay;

        s.dayTimeRemaining =
            s.isDay ? DAY_DURATION
            : NIGHT_DURATION;
    }
}

bool readMap(
    const string& filename,
    vector<vector<Cell>>& map,
    Position& startPos)
{
    ifstream f(filename);

    if (!f.is_open())
        return false;

    string line;
    int row = 0;

    while (getline(f, line) && row < MAP_SIZE)
    {
        stringstream ss(line);
        string tok;

        int col = 0;

        while (getline(ss, tok, ',') && col < MAP_SIZE)
        {
            char c = tok[0];

            if (c == '#')
                map[row][col].isObstacle = true;

            if (c == 'B')
                map[row][col].mineral = MINERAL_BLUE;

            if (c == 'Y')
                map[row][col].mineral = MINERAL_YELLOW;

            if (c == 'G')
                map[row][col].mineral = MINERAL_GREEN;

            if (c == 'S')
            {
                map[row][col].isStart = true;
                startPos = Position(row, col);
            }

            col++;
        }

        row++;
    }

    allMinerals = getAllMinerals(map);

    cout << "Minerals: "
        << allMinerals.size()
        << endl;

    return true;
}

bool isWalkable(
    int x,
    int y,
    const vector<vector<Cell>>& map)
{
    if (x < 0 || y < 0 ||
        x >= MAP_SIZE ||
        y >= MAP_SIZE)
        return false;

    return !map[x][y].isObstacle;
}

bool isMineral(
    int x,
    int y,
    const vector<vector<Cell>>& map)
{
    return map[x][y].mineral != MINERAL_NONE;
}


const int dx[] = { -1,-1,-1,0,0,1,1,1 };
const int dy[] = { -1,0,1,-1,1,-1,0,1 };

pair<vector<LogEntry>, int>
aStarSearch(
    int maxTime,
    const vector<vector<Cell>>& map,
    const Position& startPos)
{
    auto t0 = chrono::steady_clock::now();

    priority_queue<
        AStarNode,
        vector<AStarNode>,
        greater<AStarNode>> open;

    unordered_map<string, int> bestG;

    RoverState start;
    start.pos = startPos;

    start.addLogEntry(0, "START");

    int h0 = heuristic(start, maxTime, startPos);

    open.push(AStarNode(start, 0, h0));
    bestG[start.getStateId()] = 0;

    RoverState bestState;
    int bestMinerals = 0;

    int iterations = 0;

    while (!open.empty())
    {
        iterations++;

        AStarNode cur = open.top();
        open.pop();

        if (cur.state.timeElapsed >= maxTime)
            continue;

        if (cur.state.pos == startPos)
        {
            if (cur.state.totalMinerals > bestMinerals)
            {
                bestMinerals = cur.state.totalMinerals;
                bestState = cur.state;

                cout << "BEST = "
                    << bestMinerals
                    << endl;
            }
        }
        for (int i = 0; i < 8; i++)
        {
            int nx = cur.state.pos.x + dx[i];
            int ny = cur.state.pos.y + dy[i];

            if (!isWalkable(nx, ny, map))
                continue;

            for (int speed = 1; speed <= 3; speed++)
            {
                RoverState ns = cur.state;

                ns.pos = Position(nx, ny);
                ns.totalDistance++;

                int e = calculateMoveEnergy(speed, ns.isDay);

                ns.battery = max(
                    0,
                    min(MAX_BATTERY,
                        ns.battery - e));

                if (ns.battery <= 0 &&
                    !ns.isDay)
                    continue;

                updateTime(ns);

                ns.addLogEntry(speed, "MOVE");

                string id = ns.getStateId();
                int g = ns.timeElapsed;

                if (bestG.find(id) == bestG.end()
                    || g < bestG[id])
                {
                    bestG[id] = g;

                    int h =
                        heuristic(
                            ns,
                            maxTime,
                            startPos);

                    open.push(
                        AStarNode(ns, g, h));
                }
            }
        }
        if (isMineral(
            cur.state.pos.x,
            cur.state.pos.y,
            map) &&
            cur.state.collected.find(
                cur.state.pos)
            == cur.state.collected.end())
        {
            RoverState ns = cur.state;

            ns.collected.insert(ns.pos);
            ns.totalMinerals++;

            updateTime(ns);

            ns.addLogEntry(0, "MINE");

            string id = ns.getStateId();
            int g = ns.timeElapsed;

            if (bestG.find(id) == bestG.end()
                || g < bestG[id])
            {
                bestG[id] = g;

                int h =
                    heuristic(
                        ns,
                        maxTime,
                        startPos);

                open.push(
                    AStarNode(ns, g, h));
            }
        }
    }

    cout << "Iterations = "
        << iterations
        << endl;

    return make_pair(
        bestState.log,
        bestMinerals);
}

void saveLogToFile(
    const vector<LogEntry>& log,
    const string& filename)
{
    ofstream f(filename);

    f << "TimeStep,X,Y,Battery,Speed,Distance,"
        "Minerals,Action,TimeOfDay,Timestamp\n";

    for (const auto& e : log)
        f << e.toString() << "\n";
}