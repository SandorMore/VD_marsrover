#include "HeightMap.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

HeightMap::HeightMap() : width(0), height(0), startPosition(0, 0) {}

HeightMap::~HeightMap() {}

bool HeightMap::loadFromCSV(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return false;
    }

    std::vector<std::string> rows;
    std::string line;

    while (std::getline(file, line)) {
        if (!line.empty()) {
            rows.push_back(line);
        }
    }

    file.close();

    if (rows.empty()) {
        std::cerr << "Empty file" << std::endl;
        return false;
    }

    height = rows.size();
    width = rows[0].length();

    initializeGrid(width, height);

    for (int y = 0; y < height; y++) {
        const std::string& row = rows[y];
        for (int x = 0; x < width && x < row.length(); x++) {
            char c = row[x];
            TerrainType type = charToTerrainType(c);

            TerrainCell cell;
            cell.type = type;
            cell.color = getTypeColor(type);
            cell.hasMineral = (type == TerrainType::BLUE_MINERAL ||
                type == TerrainType::YELLOW_MINERAL ||
                type == TerrainType::GREEN_MINERAL);
            cell.mineralValue = cell.hasMineral ? 1 : 0;
            cell.isPassable = (type != TerrainType::OBSTACLE);
            cell.height = 0.0f;

            setCell(x, y, cell);

            if (type == TerrainType::START) {
                startPosition = glm::vec2(x, y);
            }
        }
    }

    std::cout << "Map loaded: " << width << "x" << height << std::endl;
    std::cout << "Minerals found: " << getMineralPositions().size() << std::endl;
    std::cout << "  Blue: " << countMineralType(TerrainType::BLUE_MINERAL) << std::endl;
    std::cout << "  Yellow: " << countMineralType(TerrainType::YELLOW_MINERAL) << std::endl;
    std::cout << "  Green: " << countMineralType(TerrainType::GREEN_MINERAL) << std::endl;

    return true;
}

void HeightMap::initializeGrid(int w, int h) {
    grid.resize(h, std::vector<TerrainCell>(w));
    width = w;
    height = h;
}

TerrainType HeightMap::charToTerrainType(char c) {
    switch (c) {
    case '.': return TerrainType::FLOOR;
    case '#': return TerrainType::OBSTACLE;
    case 'B': return TerrainType::BLUE_MINERAL;
    case 'Y': return TerrainType::YELLOW_MINERAL;
    case 'G': return TerrainType::GREEN_MINERAL;
    case 'S': return TerrainType::START;
    default: return TerrainType::FLOOR;
    }
}

glm::vec3 HeightMap::getTypeColor(TerrainType type) {
    switch (type) {
    case TerrainType::FLOOR:
        return glm::vec3(0.6f, 0.4f, 0.2f);
    case TerrainType::OBSTACLE:
        return glm::vec3(0.3f, 0.3f, 0.3f);
    case TerrainType::BLUE_MINERAL:
        return glm::vec3(0.2f, 0.6f, 1.0f); 
    case TerrainType::YELLOW_MINERAL:
        return glm::vec3(1.0f, 0.8f, 0.0f); 
    case TerrainType::GREEN_MINERAL:
        return glm::vec3(0.2f, 0.8f, 0.2f); 
    case TerrainType::START:
        return glm::vec3(1.0f, 0.5f, 0.0f); 
    default:
        return glm::vec3(0.5f, 0.5f, 0.5f);
    }
}

TerrainCell HeightMap::getCell(int x, int y) const {
    if (isValidPosition(x, y)) {
        return grid[y][x];
    }
    TerrainCell defaultCell;
    defaultCell.type = TerrainType::OBSTACLE;
    defaultCell.color = glm::vec3(0.3f, 0.3f, 0.3f);
    defaultCell.hasMineral = false;
    defaultCell.isPassable = false;
    defaultCell.height = 0.0f;
    return defaultCell;
}

void HeightMap::setCell(int x, int y, const TerrainCell& cell) {
    if (isValidPosition(x, y)) {
        grid[y][x] = cell;
    }
}

bool HeightMap::isValidPosition(int x, int y) const {
    return x >= 0 && x < width && y >= 0 && y < height;
}

void HeightMap::generateHeights(float baseHeight, float mineralHeightOffset) {
    // Discrete, blocky heights to avoid cracks between cells.
    // Think in terms of Minecraft-style blocks:
    //   .  -> ground level (0)
    //   B/Y/G -> low block (1)
    //   S  -> low block (1)
    //   #  -> high block / mountain (2)
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            TerrainCell& cell = grid[y][x];

            float h = baseHeight;

            switch (cell.type) {
            case TerrainType::FLOOR:
                h += 0.0f;
                break;
            case TerrainType::BLUE_MINERAL:
            case TerrainType::YELLOW_MINERAL:
            case TerrainType::GREEN_MINERAL:
                h += 1.0f;
                break;
            case TerrainType::START:
                h += 1.0f;
                break;
            case TerrainType::OBSTACLE:
                h += 2.0f;
                break;
            default:
                break;
            }

            cell.height = h;
        }
    }
}

float HeightMap::getHeightAt(float worldX, float worldZ) const {
    int x = static_cast<int>(std::round(worldX));
    int z = static_cast<int>(std::round(worldZ));

    if (isValidPosition(x, z)) {
        return grid[z][x].height;
    }
    return 0.0f;
}

glm::vec3 HeightMap::getColorAt(int x, int y) const {
    if (isValidPosition(x, y)) {
        return grid[y][x].color;
    }
    return glm::vec3(0.3f, 0.3f, 0.3f);
}

std::vector<glm::ivec2> HeightMap::getMineralPositions() const {
    std::vector<glm::ivec2> positions;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (grid[y][x].hasMineral) {
                positions.push_back(glm::ivec2(x, y));
            }
        }
    }

    return positions;
}

int HeightMap::countMineralType(TerrainType type) const {
    int count = 0;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (grid[y][x].type == type) {
                count++;
            }
        }
    }

    return count;
}