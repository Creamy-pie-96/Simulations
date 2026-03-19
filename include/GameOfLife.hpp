#pragma once

#include <vector>
#include <string>
#include <SDL2/SDL.h>

class GameOfLife
{
public:
    GameOfLife(int width, int height, int cellSize);

    void update(float dt);
    void draw(SDL_Renderer *renderer) const;

    void clear();
    void randomize(float aliveProbability = 0.16f);
    void paintCell(int x, int y, bool alive);
    void placePattern(const std::string &name, int cx, int cy);

    void increaseTickRate();
    void decreaseTickRate();

    float getTickRate() const { return tickRate; }
    int getCellSize() const { return cellSize; }
    int getCols() const { return cols; }
    int getRows() const { return rows; }
    int getAliveCount() const;
    int getFadingCount() const;

private:
    int width;
    int height;
    int cellSize;
    int cols;
    int rows;

    float tickRate;
    float accumulator;

    std::vector<float> cells;
    std::vector<float> next;

    int idx(int x, int y) const;
    int wrapX(int x) const;
    int wrapY(int y) const;
    int countNeighbors(int x, int y) const;
    void step();
};
