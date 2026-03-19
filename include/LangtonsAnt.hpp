#pragma once

#include <SDL2/SDL.h>
#include <cstdint>
#include <vector>

class LangtonsAnt
{
public:
    LangtonsAnt(int width, int height, int cellSize);

    void update(float dt);
    void draw(SDL_Renderer *renderer);
    void clear();
    void randomize();

    void addAnt();
    void addAntAtPixel(int px, int py);
    void paintCell(int px, int py, bool black);
    void increaseSpeed();
    void decreaseSpeed();

    std::uint64_t getStepCount() const { return stepCount; }
    int getAntCount() const { return static_cast<int>(ants.size()); }
    float getTickRate() const { return tickRate; }

private:
    struct Ant
    {
        int x;
        int y;
        int dir;  // 0=N,1=E,2=S,3=W
        int type; // color index
    };

    int width;
    int height;
    int cellSize;
    int cols;
    int rows;

    float tickRate;
    float accumulator;
    std::uint64_t stepCount;

    std::vector<int> cells;
    std::vector<Ant> ants;
    std::vector<SDL_Color> antColors;

    int wrapX(int x) const;
    int wrapY(int y) const;
    int idx(int x, int y) const;

    void step();
    void rebuildAntColors();
};
