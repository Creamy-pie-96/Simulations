#pragma once

#include <SDL2/SDL.h>
#include <string>
#include <vector>

class ReactionDiffusion
{
public:
    ReactionDiffusion(int width, int height, int cellSize);

    void update(float dt);
    void draw(SDL_Renderer *renderer);
    void clear();
    void randomize();
    void injectAtPixel(int px, int py);
    void eraseAtPixel(int px, int py);

    void setParams(float f, float k);
    void nextPreset();
    std::string getPresetName() const;

private:
    struct Preset
    {
        const char *name;
        float f;
        float k;
    };

    int width;
    int height;
    int cellSize;
    int cols;
    int rows;

    float dA;
    float dB;
    float f;
    float k;
    float simDt;

    float tickRate;
    float accumulator;

    int presetIndex;
    std::vector<Preset> presets;

    std::vector<float> A;
    std::vector<float> B;
    std::vector<float> nextA;
    std::vector<float> nextB;

    int wrapX(int x) const;
    int wrapY(int y) const;
    int idx(int x, int y) const;

    float laplace(const std::vector<float> &grid, int x, int y) const;
};
