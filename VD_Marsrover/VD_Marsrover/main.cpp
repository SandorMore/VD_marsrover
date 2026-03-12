#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <future>
#include <atomic>
#ifdef _WIN32
#include <windows.h>
#endif
#include "stb_easy_font.h"
#define GLM_ENABLE_EXPERIMENTAL
#include "Camera.h"
#include "HeightMap.h"
#include "TerrainRenderer.h"
#include "rover.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window, float deltaTime);
void updateDayNightCycle(float deltaTime);

// UI / dashboard helpers
void initUIDashboard();
void renderDashboard();
void cleanupUIDashboard();
void renderStartupScreen();

// Route simulation helper (defined later)
std::pair<std::vector<LogEntry>, int>
simulateRouteFromPath(const std::vector<Position>& path,
                      const std::vector<std::vector<Cell>>& map,
                      int maxTimeStepsHalfHours);

// Map loading / planning helpers
void loadMapAndStartPlanning(const std::string& mapFile);

Camera camera(glm::vec3(25.0f, 20.0f, 25.0f));
float lastX = 400.0f;
float lastY = 300.0f;
bool firstMouse = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

HeightMap g_heightMap;
TerrainRenderer g_terrainRenderer;

// Rover simulation / visualization state
std::vector<std::vector<Cell>> g_roverMap(MAP_SIZE, std::vector<Cell>(MAP_SIZE));
Position g_roverStart;
std::vector<Position> g_roverPath;
size_t g_roverPathIndex = 0;
float g_roverAnimTime = 0.0f;
const float g_roverStepDuration = 0.1f; // seconds per path step

// Full rover log for dashboard visualization
std::vector<LogEntry> g_roverLog;
size_t g_roverLogIndex = 0;

// Background planning
std::future<std::pair<std::vector<LogEntry>, int>> g_planFuture;
std::atomic<bool> g_planReady = false;
std::atomic<bool> g_planStarted = false;
int g_planMinerals = 0;

// Current map file (default)
std::string g_currentMapFile = "asd.txt";

// Mouse click state for UI buttons
bool g_leftMouseDownPrev = false;

// Simulation state
bool g_simulationRunning = false;
bool g_showStartupScreen = true;

void loadMapAndStartPlanning(const std::string& mapFile)
{
    std::cout << "Loading map: " << mapFile << std::endl;

    if (!g_heightMap.loadFromCSV(mapFile)) {
        std::cerr << "Failed to load map '" << mapFile
                  << "'. Creating default 50x50 map." << std::endl;

        std::ofstream defaultMap(mapFile);
        if (!defaultMap.is_open()) {
            std::cerr << "Could not create default map file '" << mapFile << "'." << std::endl;
            return;
        }

        for (int i = 0; i < 50; i++) {
            for (int j = 0; j < 50; j++) {
                if (i == 25 && j == 25) defaultMap << 'S';
                else if (rand() % 10 == 0) defaultMap << 'B';
                else if (rand() % 15 == 0) defaultMap << 'Y';
                else if (rand() % 20 == 0) defaultMap << 'G';
                else if (rand() % 8 == 0) defaultMap << '#';
                else defaultMap << '.';
            }
            defaultMap << '\n';
        }
        defaultMap.close();

        if (!g_heightMap.loadFromCSV(mapFile)) {
            std::cerr << "Failed to load generated default map from '" << mapFile
                      << "'." << std::endl;
            return;
        }
    }

    g_heightMap.generateHeights(0.0f, 0.5f);

    // Rebuild terrain renderer mesh for the new height map
    g_terrainRenderer.cleanup();
    if (!g_terrainRenderer.initialize(&g_heightMap)) {
        std::cerr << "Failed to initialize terrain renderer for map '" << mapFile << "'" << std::endl;
        return;
    }

    // Prepare rover logical map
    if (!readMap(mapFile, g_roverMap, g_roverStart)) {
        std::cerr << "Failed to read rover map from '" << mapFile << "'" << std::endl;
        return;
    }

    const int maxStepsFor24Hours = 24 * 2; // half-hour steps

    auto route = buildFastRoute(g_roverMap, g_roverStart);
    auto aiResult = simulateRouteFromPath(route, g_roverMap, maxStepsFor24Hours);

    g_roverLog = aiResult.first;
    g_planMinerals = aiResult.second;

    g_roverPath.clear();
    g_roverPath.reserve(g_roverLog.size());
    for (const auto& entry : g_roverLog) {
        g_roverPath.emplace_back(entry.x, entry.y);
    }
    g_roverPathIndex = 0;
    g_roverLogIndex = 0;

    g_planStarted = true;
    g_planReady = true;

    std::cout << "AI route length (log entries): " << g_roverLog.size() << std::endl;
    std::cout << "Total minerals collected along AI route: " << g_planMinerals << std::endl;
}

GLuint g_roverVAO = 0;
GLuint g_roverVBO = 0;
GLuint g_roverEBO = 0;

// UI dashboard GL objects
GLuint g_uiVAO = 0;
GLuint g_uiVBO = 0;
GLuint g_uiShaderProgram = 0;
int g_windowWidth = 1200;
int g_windowHeight = 800;

float g_timeOfDay = 6.0f;
bool g_isDay = true;
glm::vec3 g_ambientLight = glm::vec3(0.8f, 0.8f, 0.7f);
glm::vec3 g_sunColor = glm::vec3(1.0f, 0.95f, 0.9f);
glm::vec3 g_sunDirection = glm::normalize(glm::vec3(1.0f, 1.0f, 0.3f)); // direction from sun towards scene

bool g_showWireframe = false;
bool g_showGrid = true;
float g_terrainScale = 1.0f;

GLuint shaderProgram;

// --- UI / dashboard + simulation helpers ---

struct UIVertex {
    float x;
    float y;
    float r;
    float g;
    float b;
};

// Simulate rover along a precomputed path, generating log entries that
// respect the task's energy and time rules.
std::pair<std::vector<LogEntry>, int>
simulateRouteFromPath(const std::vector<Position>& path,
                      const std::vector<std::vector<Cell>>& map,
                      int maxTimeStepsHalfHours)
{
    std::vector<LogEntry> log;
    if (path.empty())
        return { log, 0 };

    RoverState state;
    state.pos = path.front();
    state.addLogEntry(0, "START");

    std::unordered_set<Position, PositionHash> mined;

    auto chooseSpeed = [&](int /*stepIndex*/) -> int {
        if (state.isDay && state.battery > 70) return 3;
        if (state.battery > 40) return 2;
        return 1;
    };

    for (size_t i = 1; i < path.size(); ++i) {
        if (state.timeElapsed >= maxTimeStepsHalfHours)
            break;

        const Position& next = path[i];

        int speed = chooseSpeed(static_cast<int>(i));
        int eMove = calculateMoveEnergy(speed, state.isDay);
        state.battery = std::max(0, std::min(MAX_BATTERY, state.battery - eMove));

        state.pos = next;
        state.totalDistance++;
        updateTime(state);
        state.addLogEntry(speed, "MOVE");

        if (state.timeElapsed >= maxTimeStepsHalfHours)
            break;

        if (isMineral(state.pos.x, state.pos.y, map) &&
            mined.find(state.pos) == mined.end())
        {
            int eMine = calculateMineEnergy(state.isDay);
            state.battery = std::max(0, std::min(MAX_BATTERY, state.battery - eMine));

            state.totalMinerals++;
            mined.insert(state.pos);

            updateTime(state);
            state.addLogEntry(0, "MINE");
        }
    }

    log = std::move(state.log);
    return { log, state.totalMinerals };
}

// Converts stb_easy_font quads to our triangle list.
void addText(std::vector<UIVertex>& verts,
             float x, float y,
             float scale,
             const std::string& text,
             float r, float g, float b)
{
    if (text.empty())
        return;

    static char buffer[200000]; // enough for a lot of text
    memset(buffer, 0, sizeof(buffer));

    std::string tmp = text;
    int numQuads = stb_easy_font_print(x, y, &tmp[0], nullptr, buffer, (int)sizeof(buffer));
    if (numQuads <= 0)
        return;

    const int vertsPerQuad = 4;
    const int stride = 16;

    for (int q = 0; q < numQuads; ++q)
    {
        float vx[4];
        float vy[4];
        for (int i = 0; i < 4; ++i)
        {
            const char* vptr = buffer + (q * vertsPerQuad + i) * stride;
            float px = *(const float*)(vptr + 0);
            float py = *(const float*)(vptr + 4);

            px = x + (px - x) * scale;
            py = y + (py - y) * scale;

            vx[i] = px;
            vy[i] = py;
        }

        verts.push_back({ vx[0], vy[0], r, g, b });
        verts.push_back({ vx[1], vy[1], r, g, b });
        verts.push_back({ vx[2], vy[2], r, g, b });

        verts.push_back({ vx[0], vy[0], r, g, b });
        verts.push_back({ vx[2], vy[2], r, g, b });
        verts.push_back({ vx[3], vy[3], r, g, b });
    }
}

// Helper to push a colored rectangle (two triangles) into a vertex buffer.
void addRect(std::vector<UIVertex>& verts,
             float x, float y, float w, float h,
             float r, float g, float b) {
    UIVertex v0 { x,     y,     r, g, b };
    UIVertex v1 { x + w, y,     r, g, b };
    UIVertex v2 { x + w, y + h, r, g, b };
    UIVertex v3 { x,     y + h, r, g, b };

    verts.push_back(v0);
    verts.push_back(v1);
    verts.push_back(v2);
    verts.push_back(v0);
    verts.push_back(v2);
    verts.push_back(v3);
}

// Simple helper to blend two colors
void lerpColor(float t,
               float r1, float g1, float b1,
               float r2, float g2, float b2,
               float& rOut, float& gOut, float& bOut) {
    t = std::max(0.0f, std::min(1.0f, t));
    rOut = r1 + (r2 - r1) * t;
    gOut = g1 + (g2 - g1) * t;
    bOut = b1 + (b2 - b1) * t;
}

void initUIDashboard() {
    if (g_uiShaderProgram != 0)
        return;

    const char* uiVertSrc = R"(
        #version 330 core
        layout (location = 0) in vec2 aPos;
        layout (location = 1) in vec3 aColor;

        out vec3 vColor;

        uniform vec2 uResolution;

        void main() {
            // Convert from pixel space to NDC (-1..1) with TOP-LEFT origin (like UI coords).
            vec2 pos = vec2(
                (aPos.x / uResolution.x) * 2.0 - 1.0,
                1.0 - (aPos.y / uResolution.y) * 2.0
            );
            gl_Position = vec4(pos, 0.0, 1.0);
            vColor = aColor;
        }
    )";

    const char* uiFragSrc = R"(
        #version 330 core
        in vec3 vColor;
        out vec4 FragColor;

        void main() {
            FragColor = vec4(vColor, 0.95);
        }
    )";

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &uiVertSrc, nullptr);
    glCompileShader(vs);

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &uiFragSrc, nullptr);
    glCompileShader(fs);

    g_uiShaderProgram = glCreateProgram();
    glAttachShader(g_uiShaderProgram, vs);
    glAttachShader(g_uiShaderProgram, fs);
    glLinkProgram(g_uiShaderProgram);

    glDeleteShader(vs);
    glDeleteShader(fs);

    glGenVertexArrays(1, &g_uiVAO);
    glGenBuffers(1, &g_uiVBO);

    glBindVertexArray(g_uiVAO);
    glBindBuffer(GL_ARRAY_BUFFER, g_uiVBO);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(UIVertex), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(UIVertex), (void*)(2 * sizeof(float)));

    glBindVertexArray(0);
}

void cleanupUIDashboard() {
    if (g_uiVBO) {
        glDeleteBuffers(1, &g_uiVBO);
        g_uiVBO = 0;
    }
    if (g_uiVAO) {
        glDeleteVertexArrays(1, &g_uiVAO);
        g_uiVAO = 0;
    }
    if (g_uiShaderProgram) {
        glDeleteProgram(g_uiShaderProgram);
        g_uiShaderProgram = 0;
    }
}

void renderStartupScreen() {
    if (!g_uiShaderProgram)
        return;

    std::vector<UIVertex> verts;
    verts.reserve(6 * 64);

    const float panelW = 520.0f;
    const float panelH = 260.0f;
    const float panelX = (static_cast<float>(g_windowWidth) - panelW) * 0.5f;
    const float panelY = (static_cast<float>(g_windowHeight) - panelH) * 0.5f;

    addRect(verts, panelX, panelY, panelW, panelH, 0.03f, 0.03f, 0.05f);

    addText(verts, panelX + 26.0f, panelY + 38.0f, 1.4f,
            "MARS ROVER SIMULATION", 0.92f, 0.95f, 1.0f);

    std::ostringstream info;
    info << "Current map: " << (g_currentMapFile.empty() ? "none" : g_currentMapFile);
    addText(verts, panelX + 26.0f, panelY + 70.0f, 1.0f,
            info.str(), 0.72f, 0.80f, 0.95f);

    addText(verts, panelX + 26.0f, panelY + 94.0f, 0.95f,
            "1. Load or confirm the map file.", 0.65f, 0.72f, 0.88f);
    addText(verts, panelX + 26.0f, panelY + 112.0f, 0.95f,
            "2. Click START SIMULATION to begin.", 0.65f, 0.72f, 0.88f);

    // Buttons
    const float btnW = 150.0f;
    const float btnH = 32.0f;

    const float loadBtnX = panelX + 26.0f;
    const float loadBtnY = panelY + panelH - 70.0f;

    const float startBtnX = panelX + panelW - btnW - 26.0f;
    const float startBtnY = loadBtnY;

    addRect(verts, loadBtnX, loadBtnY, btnW, btnH, 0.10f, 0.18f, 0.32f);
    addText(verts, loadBtnX + 18.0f, loadBtnY + 11.0f, 0.95f,
            "LOAD MAP", 0.90f, 0.95f, 1.0f);

    glm::vec3 startCol = g_roverLog.empty()
        ? glm::vec3(0.25f, 0.25f, 0.30f)
        : glm::vec3(0.12f, 0.42f, 0.25f);
    addRect(verts, startBtnX, startBtnY, btnW, btnH,
            startCol.r, startCol.g, startCol.b);
    addText(verts, startBtnX + 12.0f, startBtnY + 11.0f, 0.95f,
            "START SIMULATION", 0.92f, 0.98f, 0.96f);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(g_uiShaderProgram);
    glBindVertexArray(g_uiVAO);
    glBindBuffer(GL_ARRAY_BUFFER, g_uiVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(verts.size() * sizeof(UIVertex)),
                 verts.data(),
                 GL_DYNAMIC_DRAW);

    GLint resLoc = glGetUniformLocation(g_uiShaderProgram, "uResolution");
    if (resLoc >= 0) {
        float res[2] = { static_cast<float>(g_windowWidth), static_cast<float>(g_windowHeight) };
        glUniform2fv(resLoc, 1, res);
    }

    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(verts.size()));

    glBindVertexArray(0);
    glUseProgram(0);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

void renderDashboard() {
    if (!g_uiShaderProgram)
        return;

    std::vector<UIVertex> verts;
    verts.reserve(6 * 128); // rectangles + text

    // Loading screen if plan is not ready yet (during simulation)
    if (!g_planReady.load())
    {
        const float panelW = 560.0f;
        const float panelH = 180.0f;
        const float panelX = (static_cast<float>(g_windowWidth) - panelW) * 0.5f;
        const float panelY = (static_cast<float>(g_windowHeight) - panelH) * 0.5f;

        addRect(verts, panelX, panelY, panelW, panelH, 0.04f, 0.04f, 0.06f);

        // Animated "progress" bar using time
        float t = static_cast<float>(glfwGetTime());
        float wave = 0.5f + 0.5f * sin(t * 2.5f);

        float barX = panelX + 40.0f;
        float barY = panelY + panelH - 60.0f;
        float barW = panelW - 80.0f;
        float barH = 14.0f;

        addRect(verts, barX, barY, barW, barH, 0.10f, 0.12f, 0.18f);
        addRect(verts, barX, barY, barW * (0.15f + 0.75f * wave), barH, 0.35f, 0.78f, 1.0f);

        addText(verts, panelX + 40.0f, panelY + 40.0f, 1.6f, "PLANNING ROUTE (AI)...", 0.9f, 0.92f, 1.0f);
        addText(verts, panelX + 40.0f, panelY + 78.0f, 1.2f, "Generating log, battery & day/night timeline", 0.65f, 0.7f, 0.82f);

        // Draw UI
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glUseProgram(g_uiShaderProgram);
        glBindVertexArray(g_uiVAO);
        glBindBuffer(GL_ARRAY_BUFFER, g_uiVBO);
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(verts.size() * sizeof(UIVertex)),
                     verts.data(),
                     GL_DYNAMIC_DRAW);

        GLint resLoc = glGetUniformLocation(g_uiShaderProgram, "uResolution");
        if (resLoc >= 0) {
            float res[2] = { static_cast<float>(g_windowWidth), static_cast<float>(g_windowHeight) };
            glUniform2fv(resLoc, 1, res);
        }

        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(verts.size()));

        glBindVertexArray(0);
        glUseProgram(0);

        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        return;
    }

    if (g_roverLog.empty())
        return;

    const LogEntry& entry = g_roverLog[g_roverLogIndex];

    const float panelX = 20.0f;
    const float panelY = 20.0f;
    const float panelW = 360.0f;
    const float panelH = 200.0f;

    // Background panel
    addRect(verts, panelX, panelY, panelW, panelH, 0.04f, 0.04f, 0.06f);

    // "Load map" button in the top-right of the panel
    const float loadBtnW = 90.0f;
    const float loadBtnH = 22.0f;
    const float loadBtnX = panelX + panelW - loadBtnW - 16.0f;
    const float loadBtnY = panelY + 18.0f;
    addRect(verts, loadBtnX, loadBtnY, loadBtnW, loadBtnH, 0.10f, 0.18f, 0.32f);
    addText(verts, loadBtnX + 10.0f, loadBtnY + 7.0f, 0.9f, "LOAD MAP", 0.9f, 0.95f, 1.0f);

    // Title and basic info text
    {
        std::ostringstream os;
        os << "MARS ROVER DASHBOARD";
        addText(verts, panelX + 18.0f, panelY + 26.0f, 1.2f, os.str(), 0.92f, 0.95f, 1.0f);

        std::ostringstream os2;
        os2 << (entry.isDay ? "DAY" : "NIGHT") << "  " << entry.timestamp
            << "  |  Pos (" << entry.x << "," << entry.y << ")"
            << "  |  " << entry.action << " v=" << entry.speed;
        addText(verts, panelX + 18.0f, panelY + 50.0f, 1.0f, os2.str(), 0.70f, 0.78f, 0.92f);
    }

    // Battery bar frame (slightly inset)
    float batteryX = panelX + 20.0f;
    float batteryY = panelY + panelH - 40.0f;
    float batteryW = panelW - 40.0f;
    float batteryH = 18.0f;

    addText(verts, batteryX, batteryY - 14.0f, 0.95f, "BATTERY", 0.85f, 0.9f, 1.0f);
    {
        std::ostringstream os;
        os << entry.battery << "%";
        addText(verts, batteryX + batteryW - 40.0f, batteryY - 14.0f, 0.95f, os.str(), 0.85f, 0.9f, 1.0f);
    }

    addRect(verts, batteryX - 2.0f, batteryY - 2.0f, batteryW + 4.0f, batteryH + 4.0f,
            0.15f, 0.15f, 0.20f);

    float batteryPct = std::max(0.0f, std::min(100.0f, static_cast<float>(entry.battery))) / 100.0f;

    // Battery color: red -> yellow -> green
    float rMid, gMid, bMid;
    if (batteryPct < 0.5f) {
        lerpColor(batteryPct / 0.5f,
                  0.8f, 0.1f, 0.1f,   // red
                  0.95f, 0.8f, 0.2f,  // yellow
                  rMid, gMid, bMid);
    } else {
        lerpColor((batteryPct - 0.5f) / 0.5f,
                  0.95f, 0.8f, 0.2f,  // yellow
                  0.2f, 0.85f, 0.35f, // green
                  rMid, gMid, bMid);
    }

    addRect(verts, batteryX, batteryY, batteryW * batteryPct, batteryH, rMid, gMid, bMid);

    // Time-of-day strip (sun path) just below battery
    float timeStripX = batteryX;
    float timeStripY = batteryY - 32.0f;
    float timeStripW = batteryW;
    float timeStripH = 10.0f;

    addText(verts, timeStripX, timeStripY - 14.0f, 0.95f, "TIME (24h)", 0.75f, 0.82f, 0.95f);

    // Base night gradient
    addRect(verts, timeStripX, timeStripY, timeStripW, timeStripH,
            0.06f, 0.08f, 0.18f);

    // Overlay a "current time" marker
    float timeNorm = std::max(0.0f, std::min(24.0f, static_cast<float>(entry.timeStep) * 0.5f)) / 24.0f;
    float markerX = timeStripX + timeStripW * timeNorm - 2.0f;

    float dayR = entry.isDay ? 1.0f : 0.4f;
    float dayG = entry.isDay ? 0.85f : 0.5f;
    float dayB = entry.isDay ? 0.4f : 0.9f;

    addRect(verts, markerX, timeStripY - 2.0f, 4.0f, timeStripH + 4.0f, dayR, dayG, dayB);

    // Minerals progress bar
    float mineralsX = batteryX;
    float mineralsY = panelY + 96.0f;
    float mineralsW = batteryW;
    float mineralsH = 12.0f;

    {
        std::ostringstream os;
        os << "MINERALS " << entry.mineralsCollected << "/" << g_roverLog.back().mineralsCollected;
        addText(verts, mineralsX, mineralsY - 14.0f, 0.95f, os.str(), 0.75f, 0.95f, 1.0f);
    }

    addRect(verts, mineralsX, mineralsY, mineralsW, mineralsH,
            0.10f, 0.12f, 0.18f);

    int currentMinerals = entry.mineralsCollected;
    int maxMinerals = g_roverLog.back().mineralsCollected;
    float mineralsPct = (maxMinerals > 0)
                            ? std::max(0.0f, std::min(1.0f, static_cast<float>(currentMinerals) / static_cast<float>(maxMinerals)))
                            : 0.0f;

    addRect(verts, mineralsX, mineralsY, mineralsW * mineralsPct, mineralsH,
            0.3f, 0.8f, 1.0f);

    // Subtle accent to indicate whether the rover is currently moving or mining
    float statusX = panelX + panelW - 40.0f;
    float statusY = panelY + panelH - 40.0f;
    float statusSize = 18.0f;

    float sr = 0.5f, sg = 0.5f, sb = 0.5f;
    if (entry.action == "MOVE") {
        sr = 0.2f; sg = 0.7f; sb = 1.0f;
    } else if (entry.action == "MINE") {
        sr = 1.0f; sg = 0.6f; sb = 0.2f;
    } else if (entry.action == "WAIT") {
        sr = 0.7f; sg = 0.7f; sb = 0.7f;
    }
    addRect(verts, statusX, statusY, statusSize, statusSize, sr, sg, sb);

    // Slight glow under the status indicator
    addRect(verts, statusX - 4.0f, statusY - 4.0f, statusSize + 8.0f, 4.0f,
            sr * 0.3f, sg * 0.3f, sb * 0.4f);

    // Upload and draw UI
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(g_uiShaderProgram);

    glBindVertexArray(g_uiVAO);
    glBindBuffer(GL_ARRAY_BUFFER, g_uiVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(verts.size() * sizeof(UIVertex)),
                 verts.data(),
                 GL_DYNAMIC_DRAW);

    GLint resLoc = glGetUniformLocation(g_uiShaderProgram, "uResolution");
    if (resLoc >= 0) {
        float res[2] = { static_cast<float>(g_windowWidth), static_cast<float>(g_windowHeight) };
        glUniform2fv(resLoc, 1, res);
    }

    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(verts.size()));

    glBindVertexArray(0);
    glUseProgram(0);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

// Simple white quad used to visualize the rover as a box
void setupRoverMesh() {
    if (g_roverVAO != 0) return;

    float size = 0.4f;
    float y = 0.0f;

    // position (3) + normal (3) + color (3), white color, normal up
    float vertices[] = {
        -size, y, -size,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f, 1.0f,
         size, y, -size,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f, 1.0f,
         size, y,  size,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f, 1.0f,
        -size, y,  size,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f, 1.0f
    };

    unsigned int indices[] = {
        0, 1, 2,
        0, 2, 3
    };

    glGenVertexArrays(1, &g_roverVAO);
    glGenBuffers(1, &g_roverVBO);
    glGenBuffers(1, &g_roverEBO);

    glBindVertexArray(g_roverVAO);

    glBindBuffer(GL_ARRAY_BUFFER, g_roverVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_roverEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

void cleanupRoverMesh() {
    if (g_roverVAO) {
        glDeleteVertexArrays(1, &g_roverVAO);
        glDeleteBuffers(1, &g_roverVBO);
        glDeleteBuffers(1, &g_roverEBO);
        g_roverVAO = g_roverVBO = g_roverEBO = 0;
    }
}

int main(int argc, char** argv) {
    // Default map file is asd.txt, unless overridden by command-line argument
    g_currentMapFile = "asd.txt";
    if (argc > 1) {
        g_currentMapFile = argv[1];
    }

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(g_windowWidth, g_windowHeight, "Mars Terrain Visualization", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        glfwTerminate();
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    loadMapAndStartPlanning(g_currentMapFile);

    // Setup rover mesh for visualization
    setupRoverMesh();

    // Initialize UI / dashboard rendering pipeline
    initUIDashboard();

    const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aNormal;
        layout (location = 2) in vec3 aColor;
        
        out vec3 vColor;
        out vec3 vFragPos;
        out vec3 vNormal;
        
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        
        void main() {
            vec4 worldPos = model * vec4(aPos, 1.0);
            vFragPos = worldPos.xyz;
            vNormal = mat3(transpose(inverse(model))) * aNormal;
            vColor = aColor;
            gl_Position = projection * view * worldPos;
        }
    )";

    const char* fragmentShaderSource = R"(
        #version 330 core
        in vec3 vColor;
        in vec3 vFragPos;
        in vec3 vNormal;
        out vec4 FragColor;
        
        uniform vec3 ambientLight;
        uniform vec3 sunColor;
        uniform vec3 sunDirection; // direction from sun towards scene
        uniform bool isDay;
        uniform float timeOfDay;
        
        void main() {
            vec3 color = vColor;
            
            // Normalize normal and light direction
            vec3 N = normalize(vNormal);
            vec3 L = normalize(-sunDirection); // from fragment towards sun
            
            // Diffuse term
            float diff = max(dot(N, L), 0.0);
            
            // Ambient and directional lighting
            vec3 ambient = ambientLight * color;
            vec3 diffuse = sunColor * diff * color;
            
            // Extra tints for dawn/dusk and night
            if (!isDay) {
                color = mix(color, vec3(0.15, 0.2, 0.4), 0.4);
            }
            
            float dawnFactor = 1.0 - abs(timeOfDay - 6.0) / 6.0;
            if (timeOfDay > 4.0 && timeOfDay < 8.0) {
                color = mix(color, vec3(1.0, 0.5, 0.25), clamp(dawnFactor, 0.0, 1.0) * 0.5);
            }
            
            vec3 finalColor = ambient + diffuse;
            // Slightly modulate with tinted base color
            finalColor *= mix(vec3(1.0), color, 0.3);
            
            FragColor = vec4(finalColor, 1.0);
        }
    )";

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);

    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "Vertex shader compilation failed: " << infoLog << std::endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "Fragment shader compilation failed: " << infoLog << std::endl;
    }

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "Shader program linking failed: " << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    std::cout << "\n=== Mars Terrain Loaded ===" << std::endl;
    std::cout << "Map size: " << g_heightMap.getWidth() << " x " << g_heightMap.getHeight() << std::endl;
    std::cout << "Starting position: (" << g_heightMap.getStartPosition().x << ", " << g_heightMap.getStartPosition().y << ")" << std::endl;

    auto minerals = g_heightMap.getMineralPositions();
    std::cout << "Total minerals: " << minerals.size() << std::endl;
    std::cout << "  Blue (B): " << g_heightMap.countMineralType(TerrainType::BLUE_MINERAL) << std::endl;
    std::cout << "  Yellow (Y): " << g_heightMap.countMineralType(TerrainType::YELLOW_MINERAL) << std::endl;
    std::cout << "  Green (G): " << g_heightMap.countMineralType(TerrainType::GREEN_MINERAL) << std::endl;
    std::cout << "===========================\n" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  WASD - Move camera" << std::endl;
    std::cout << "  Mouse - Look around" << std::endl;
    std::cout << "  Scroll - Zoom in/out" << std::endl;
    std::cout << "  F1 - Toggle wireframe mode" << std::endl;
    std::cout << "  F2 - Reset camera to top view" << std::endl;
    std::cout << "  F3 - Toggle day/night cycle" << std::endl;
    std::cout << "  ESC - Exit" << std::endl;
    std::cout << "===========================\n" << std::endl;

    float simulationSpeed = 1.0f;
    bool isPaused = true;

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window, deltaTime);

        // Handle UI mouse click for startup & HUD buttons
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);
        int mouseState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
        bool clicked = (mouseState == GLFW_PRESS && !g_leftMouseDownPrev);
        g_leftMouseDownPrev = (mouseState == GLFW_PRESS);

        if (clicked) {
            // Startup screen buttons
            if (g_showStartupScreen) {
                const float panelW = 520.0f;
                const float panelH = 260.0f;
                const float panelX = (static_cast<float>(g_windowWidth) - panelW) * 0.5f;
                const float panelY = (static_cast<float>(g_windowHeight) - panelH) * 0.5f;
                const float btnW = 150.0f;
                const float btnH = 32.0f;
                const float loadBtnX = panelX + 26.0f;
                const float loadBtnY = panelY + panelH - 70.0f;
                const float startBtnX = panelX + panelW - btnW - 26.0f;
                const float startBtnY = loadBtnY;

                if (mouseX >= loadBtnX && mouseX <= loadBtnX + btnW &&
                    mouseY >= loadBtnY && mouseY <= loadBtnY + btnH) {
#ifdef _WIN32
                char fileBuffer[MAX_PATH] = { 0 };
                OPENFILENAMEA ofn;
                ZeroMemory(&ofn, sizeof(ofn));
                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = nullptr;
                ofn.lpstrFile = fileBuffer;
                ofn.nMaxFile = MAX_PATH;
                ofn.lpstrFilter = "CSV map files\0*.txt;*.csv\0All files\0*.*\0";
                ofn.nFilterIndex = 1;
                ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

                // Default file is the current map
                if (!g_currentMapFile.empty()) {
                    strncpy_s(fileBuffer, g_currentMapFile.c_str(), MAX_PATH - 1);
                }

                if (GetOpenFileNameA(&ofn) == TRUE) {
                    g_currentMapFile = ofn.lpstrFile;
                    loadMapAndStartPlanning(g_currentMapFile);
                }
#endif
                } else if (!g_roverLog.empty() &&
                           mouseX >= startBtnX && mouseX <= startBtnX + btnW &&
                           mouseY >= startBtnY && mouseY <= startBtnY + btnH) {
                    g_showStartupScreen = false;
                    g_simulationRunning = true;
                    isPaused = false;
                    firstMouse = true;
                    lastX = g_windowWidth * 0.5f;
                    lastY = g_windowHeight * 0.5f;
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                }
            } else {
                // In-simulation HUD "LOAD MAP" button (top-right of dashboard)
                const float panelX = 20.0f;
                const float panelY = 20.0f;
                const float panelW = 360.0f;
                const float loadBtnW = 90.0f;
                const float loadBtnH = 22.0f;
                const float loadBtnX = panelX + panelW - loadBtnW - 16.0f;
                const float loadBtnY = panelY + 18.0f;

                if (mouseX >= loadBtnX && mouseX <= loadBtnX + loadBtnW &&
                    mouseY >= loadBtnY && mouseY <= loadBtnY + loadBtnH) {
#ifdef _WIN32
                    char fileBuffer[MAX_PATH] = { 0 };
                    OPENFILENAMEA ofn;
                    ZeroMemory(&ofn, sizeof(ofn));
                    ofn.lStructSize = sizeof(ofn);
                    ofn.hwndOwner = nullptr;
                    ofn.lpstrFile = fileBuffer;
                    ofn.nMaxFile = MAX_PATH;
                    ofn.lpstrFilter = "CSV map files\0*.txt;*.csv\0All files\0*.*\0";
                    ofn.nFilterIndex = 1;
                    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

                    if (!g_currentMapFile.empty()) {
                        strncpy_s(fileBuffer, g_currentMapFile.c_str(), MAX_PATH - 1);
                    }

                    if (GetOpenFileNameA(&ofn) == TRUE) {
                        g_currentMapFile = ofn.lpstrFile;
                        loadMapAndStartPlanning(g_currentMapFile);
                    }
#endif
                }
            }
        }

        // If we're still on the startup screen, only draw that UI
        if (g_showStartupScreen) {
            glClearColor(0.02f, 0.02f, 0.03f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            renderStartupScreen();
            glfwSwapBuffers(window);
            glfwPollEvents();
            continue;
        }

        if (!isPaused && g_simulationRunning) {
            updateDayNightCycle(deltaTime * simulationSpeed);
        }

        // Smooth, continuous sky color over a full Martian sol.
        float tDay = g_timeOfDay / 24.0f;                   // 0..1
        float angle = tDay * 6.2831853f;                    // 0..2π
        float elev = sin(angle);                            // -1..1

        glm::vec3 daySky   = glm::vec3(0.45f, 0.68f, 1.0f);
        glm::vec3 duskSky  = glm::vec3(0.8f, 0.45f, 0.25f);
        glm::vec3 nightSky = glm::vec3(0.03f, 0.04f, 0.08f);

        float dayFactor = glm::clamp((elev + 0.1f) / 1.1f, 0.0f, 1.0f);
        float duskFactor = 1.0f - fabs(elev) * 1.3f;
        duskFactor = glm::clamp(duskFactor, 0.0f, 1.0f);

        glm::vec3 skyColor = glm::mix(nightSky, daySky, dayFactor);
        skyColor = glm::mix(skyColor, duskSky, duskFactor * 0.7f);

        glClearColor(skyColor.r, skyColor.g, skyColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
                                                static_cast<float>(g_windowWidth) / static_cast<float>(g_windowHeight),
                                                0.1f,
                                                200.0f);
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 terrainModel = glm::mat4(1.0f);

        float terrainWidth = g_terrainRenderer.getWidth();
        float terrainDepth = g_terrainRenderer.getDepth();
        terrainModel = glm::translate(terrainModel, glm::vec3(-terrainWidth / 2.0f, 0.0f, -terrainDepth / 2.0f));
        terrainModel = glm::scale(terrainModel, glm::vec3(g_terrainScale));

        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, &view[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &terrainModel[0][0]);

        glUniform3fv(glGetUniformLocation(shaderProgram, "ambientLight"), 1, &g_ambientLight[0]);
        glUniform3fv(glGetUniformLocation(shaderProgram, "sunColor"), 1, &g_sunColor[0]);
        glUniform3fv(glGetUniformLocation(shaderProgram, "sunDirection"), 1, &g_sunDirection[0]);
        glUniform1i(glGetUniformLocation(shaderProgram, "isDay"), g_isDay);
        glUniform1f(glGetUniformLocation(shaderProgram, "timeOfDay"), g_timeOfDay);

        // Draw terrain
        g_terrainRenderer.render(view, projection, g_showWireframe);

        // Animate rover along planned path as a small white box
        if (g_simulationRunning && !g_roverPath.empty()) {
            g_roverAnimTime += deltaTime;
            while (g_roverAnimTime >= g_roverStepDuration && g_roverPathIndex + 1 < g_roverPath.size()) {
                g_roverAnimTime -= g_roverStepDuration;
                g_roverPathIndex++;
            }

            // Keep dashboard log index in sync with current path step if possible
            if (!g_roverLog.empty()) {
                g_roverLogIndex = std::min(g_roverPathIndex, g_roverLog.size() - 1);
            }

            const Position& p = g_roverPath[g_roverPathIndex];

            // The rover logic uses (x,y) as map indices; map[row][col] with row=x, col=y.
            // Map that to terrain local coordinates: x -> z, y -> x so it aligns with HeightMap.
            float cellX = static_cast<float>(p.y);
            float cellZ = static_cast<float>(p.x);
            float cellHeight = g_heightMap.getHeightAt(cellX, cellZ) + 0.3f;

            glm::mat4 roverModel = terrainModel;
            roverModel = glm::translate(roverModel, glm::vec3(cellX, cellHeight, cellZ));

            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &roverModel[0][0]);

            glBindVertexArray(g_roverVAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }

        // Render simple coordinate grid if enabled
        if (g_showGrid) {
            // Simple grid rendering would go here
            // You could add grid lines to show cell boundaries
        }

        // Draw 2D HUD / dashboard overlay
        renderDashboard();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    g_terrainRenderer.cleanup();
    cleanupRoverMesh();
    cleanupUIDashboard();
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window, float deltaTime) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (!g_simulationRunning)
        return;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(CameraMovement::FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(CameraMovement::BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(CameraMovement::LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(CameraMovement::RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        camera.ProcessKeyboard(CameraMovement::UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        camera.ProcessKeyboard(CameraMovement::DOWN, deltaTime);


    static bool f1Pressed = false;
    if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS && !f1Pressed) {
        g_showWireframe = !g_showWireframe;
        f1Pressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_RELEASE) {
        f1Pressed = false;
    }

    static bool f2Pressed = false;
    if (glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS && !f2Pressed) {
        camera.Position = glm::vec3(25.0f, 40.0f, 25.0f);
        camera.Yaw = -90.0f;
        camera.Pitch = -45.0f;
        camera.updateCameraVectors();
        f2Pressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_F2) == GLFW_RELEASE) {
        f2Pressed = false;
    }

    static bool f3Pressed = false;
    if (glfwGetKey(window, GLFW_KEY_F3) == GLFW_PRESS && !f3Pressed) {
        g_showGrid = !g_showGrid;
        f3Pressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_F3) == GLFW_RELEASE) {
        f3Pressed = false;
    }

    if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_KP_ADD) == GLFW_PRESS) {
    }

    if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS) {
    }
}

void updateDayNightCycle(float deltaTime) {
    g_timeOfDay += deltaTime * 0.5f;

    if (g_timeOfDay >= 24.0f) {
        g_timeOfDay = 0.0f;
    }

    // Compute sun direction as it orbits around the scene over a 24h sol.
    // Use an inclined circular path: high in the sky at noon, below horizon at night.
    float angle = glm::radians((g_timeOfDay / 24.0f) * 360.0f);
    float elevation = sin(angle);     // -1..1
    float horizontal = cos(angle);    // -1..1

    // Slight tilt in Z to avoid perfectly symmetric lighting
    g_sunDirection = glm::normalize(glm::vec3(horizontal, elevation, 0.3f));

    // Smooth day/night factor from sun elevation.
    // elevation ~ 1   -> full day
    // elevation ~ 0   -> dusk/dawn
    // elevation ~ -1  -> full night
    float dayFactor = glm::clamp((elevation + 0.15f) / 1.15f, 0.0f, 1.0f);
    float nightFactor = 1.0f - dayFactor;

    // For other logic we still expose a boolean, but derived smoothly.
    g_isDay = dayFactor > 0.35f;

    // Base colors
    glm::vec3 dayAmbient  = glm::vec3(0.7f, 0.65f, 0.6f);
    glm::vec3 nightAmbient = glm::vec3(0.05f, 0.06f, 0.10f);

    glm::vec3 warmSun = glm::vec3(1.0f, 0.85f, 0.65f);
    glm::vec3 whiteSun = glm::vec3(1.05f, 1.0f, 0.95f);
    glm::vec3 moonBlue = glm::vec3(0.1f, 0.15f, 0.35f);

    // Dusk/dawn warm accent strongest near horizon (elevation ~ 0).
    float duskFactor = 1.0f - fabs(elevation) * 1.3f;
    duskFactor = glm::clamp(duskFactor, 0.0f, 1.0f);

    // Ambient smoothly blends between night and day.
    float ambIntensityDay = 0.4f + 0.4f * dayFactor;
    float ambIntensityNight = 0.2f + 0.3f * nightFactor;
    g_ambientLight =
        glm::mix(nightAmbient * ambIntensityNight,
                 dayAmbient * ambIntensityDay,
                 dayFactor);

    // Sun color: blue and dim at night, warm/white at day, with extra orange at dusk/dawn.
    glm::vec3 daySun = glm::mix(warmSun, whiteSun, dayFactor);
    glm::vec3 baseSun = glm::mix(moonBlue * 0.6f, daySun, dayFactor);
    glm::vec3 duskTint = glm::vec3(1.1f, 0.6f, 0.3f);
    g_sunColor = glm::mix(baseSun, duskTint, duskFactor * 0.6f);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    g_windowWidth = width;
    g_windowHeight = height;
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xOffset = xpos - lastX;
    float yOffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xOffset, yOffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    camera.ProcessMouseScroll(yoffset);
}