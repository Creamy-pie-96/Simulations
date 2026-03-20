#pragma once

#include <SDL2/SDL.h>
#include <string>
#include <vector>

class MazeArt
{
public:
    MazeArt(int width, int height, int cell = 8);

    void update(float dt);
    void draw(SDL_Renderer *renderer);

    void regenerate(bool clearImageWeights = false);
    bool loadImage(const std::string &path);
    void unloadImage();

    void increaseSpeed();
    void decreaseSpeed();
    void toggleMonochrome();

    int getCols() const { return cols; }
    int getRows() const { return rows; }
    int getVisitedCount() const { return visitedCount; }
    int getTotalCells() const { return cols * rows; }
    float getSpeed() const { return stepsPerFrame; }
    bool hasImage() const { return imageLoaded; }
    std::string getImagePath() const { return imagePath; }
    bool getMonochrome() const { return monochrome; }
    bool isGenerating() const { return !stack.empty(); }

private:
    struct Cell
    {
        bool visited = false;
        bool walls[4]{true, true, true, true}; // N E S W
        SDL_Color color{255, 255, 255, 255};
    };

    struct Pixel
    {
        Uint8 r = 255, g = 255, b = 255;
    };

    int width;
    int height;
    int cellSize;
    int cols;
    int rows;

    float stepsPerFrame;
    float stepAccumulator;
    int visitedCount;

    std::vector<Cell> grid;
    std::vector<int> stack;

    bool imageLoaded;
    std::string imagePath;
    std::vector<Pixel> imageMap;
    bool monochrome;

    int idx(int x, int y) const { return y * cols + x; }
    bool inBounds(int x, int y) const { return x >= 0 && x < cols && y >= 0 && y < rows; }

    void seedRandomColors();
    void carveStep();
    int pickWeightedNeighbor(int cx, int cy, const std::vector<int> &neighbors) const;
};
