#pragma once
#include <vector>
#include <string>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>

enum class TerrainType {
    FLOOR = 0,      
    OBSTACLE = 1,  
    BLUE_MINERAL = 2,   
    YELLOW_MINERAL = 3, 
    GREEN_MINERAL = 4,  
    START = 5     
};

struct TerrainCell {
    TerrainType type;
    float height;           
    glm::vec3 color;        
    bool hasMineral;        
    int mineralValue;       
    bool isPassable;       
};

class HeightMap {
public:
    HeightMap();
    ~HeightMap();

    bool loadFromCSV(const std::string& filename);

    TerrainCell getCell(int x, int y) const;
    void setCell(int x, int y, const TerrainCell& cell);

    int getWidth() const { return width; }
    int getHeight() const { return height; }

    glm::vec2 getStartPosition() const { return startPosition; }

    void generateHeights(float baseHeight = 0.0f, float mineralHeightOffset = 0.5f);

    bool isValidPosition(int x, int y) const;

    float getHeightAt(float worldX, float worldZ) const;

    glm::vec3 getColorAt(int x, int y) const;

    std::vector<glm::ivec2> getMineralPositions() const;

    int countMineralType(TerrainType type) const;

private:
    std::vector<std::vector<TerrainCell>> grid;
    int width;
    int height;
    glm::vec2 startPosition;

    TerrainType charToTerrainType(char c);

    glm::vec3 getTypeColor(TerrainType type);

    void initializeGrid(int w, int h);
};