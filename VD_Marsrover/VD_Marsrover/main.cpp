#include "rover.h"
#include <iostream>
#include <string>
#include <cstdlib>
#include <ctime>
#include <thread>
#include "misc.h"

#define WIDTH 1200
#define HEIGHT 800

using namespace std;

//rover_log_1771854750.csv

#define MAX_HOURS_TSET 100

string mapFile = R"(C:\Users\Sanyi\Desktop\verseny\VD_Marsrover\VD_Marsrover\asd.txt)";

constexpr int maxHours = MAX_HOURS_TSET; //atoi(argv[2])
constexpr int maxHalfHours = maxHours * 2;

void CoreLoop(const char* title);

int main(int argc, char* argv[]) {

    std::thread t();
    
    CoreLoop("Marsrover");


    /*if (t.joinable()) {
        t.join();
    }
    return 0;*/
}


void CoreLoop(const char* title) {
    
    InitWindow(WIDTH, HEIGHT, title);
    
    unsigned ch_to_draw = 10;

    while (!WindowShouldClose()) {
        BeginDrawing();

        ClearBackground(DARKPURPLE);
        
        for (size_t i = 3; i < 8; ++i) {
            //DrawText("Sigma", ch_to_draw , i * 30, 32, GREEN);
            cout << "Mars Rover Simulation" << endl;
            cout << "=====================" << endl;
            cout << "Map file: " << mapFile << endl;
            cout << "Time limit: " << maxHours << " hours (" << maxHalfHours << " half-hours)" << endl;
            cout << endl;

            vector<vector<Cell>> map(MAP_SIZE, vector<Cell>(MAP_SIZE));
            Position startPos;

            if (!readMap(mapFile, map, startPos)) {
                return;
            }

            cout << "Start position: (" << startPos.x << "," << startPos.y << ")" << endl;
            cout << "Searching for optimal path..." << endl;
            cout << endl;

            pair<vector<LogEntry>, int> result = aStarSearch(maxHalfHours, map, startPos);
            vector<LogEntry> log = result.first;
            int minerals = result.second;

            cout << "=== RESULTS ===" << endl;
            cout << "Minerals collected: " << minerals << endl;
            cout << "Log entries: " << log.size() << endl;

            if (!log.empty()) {
                time_t now = time(nullptr);
                string logFilename = "rover_log_" + to_string(now) + ".csv";
                saveLogToFile(log, logFilename);

                const LogEntry& last = log.back();

                cout << "\n=== STATISTICS ===" << endl;
                cout << "Final position: (" << last.x << "," << last.y << ")" << endl;
                cout << "Final battery: " << last.battery << "%" << endl;
                cout << "Total distance: " << last.distance << " blocks" << endl;
                cout << "Time elapsed: " << last.timestamp << endl;

                if (last.x == startPos.x && last.y == startPos.y) {
                    cout << "Returned to start: YES" << endl;
                }
                else {
                    cout << "Returned to start: NO" << endl;
                }

                int daySteps = 0, nightSteps = 0;
                int moveSteps = 0, mineSteps = 0, waitSteps = 0;
                int totalEnergy = 0;

                for (size_t i = 0; i < log.size(); i++) {
                    if (log[i].isDay) daySteps++;
                    else nightSteps++;

                    if (log[i].action.find("MOVE") != string::npos) {
                        moveSteps++;
                        size_t pos = log[i].action.find_last_of('_');
                        if (pos != string::npos) {
                            int speed = stoi(log[i].action.substr(pos + 1));
                            totalEnergy += calculateMoveEnergy(speed, log[i].isDay);
                        }
                    }
                    else if (log[i].action == "MINE") {
                        mineSteps++;
                        totalEnergy += calculateMineEnergy(log[i].isDay);
                    }
                    else if (log[i].action == "WAIT") {
                        waitSteps++;
                        totalEnergy += calculateWaitEnergy(log[i].isDay);
                    }
                }

                cout << "Day steps: " << daySteps << endl;
                cout << "Night steps: " << nightSteps << endl;
                cout << "Moves: " << moveSteps << endl;
                cout << "Mining: " << mineSteps << endl;
                cout << "Waiting: " << waitSteps << endl;
                cout << "Total energy change: " << totalEnergy << endl;

                int minBattery = MAX_BATTERY, maxBattery = 0;
                for (const auto& entry : log) {
                    minBattery = min(minBattery, entry.battery);
                    maxBattery = max(maxBattery, entry.battery);
                }
                cout << "Min battery: " << minBattery << "%" << endl;
                cout << "Max battery: " << maxBattery << "%" << endl;

            }
            else {
                cout << "No valid path found!" << endl;
            }
        }
        EndDrawing();
    }
}