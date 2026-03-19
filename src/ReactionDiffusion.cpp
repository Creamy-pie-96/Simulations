#include "ReactionDiffusion.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>

static SDL_Color lerpColorRD(const SDL_Color &a, const SDL_Color &b, float t)
{
    t = std::clamp(t, 0.0f, 1.0f);
    SDL_Color c;
    c.r = static_cast<Uint8>(a.r + (b.r - a.r) * t);
    c.g = static_cast<Uint8>(a.g + (b.g - a.g) * t);
    c.b = static_cast<Uint8>(a.b + (b.b - a.b) * t);
    c.a = 255;
    return c;
}

ReactionDiffusion::ReactionDiffusion(int width, int height, int cellSize)
    : width(width), height(height), cellSize(cellSize),
      dA(1.0f), dB(0.5f), f(0.055f), k(0.062f), simDt(1.0f),
      tickRate(60.0f), accumulator(0.0f), presetIndex(0)
{
    cols = width / cellSize;
    rows = height / cellSize;

    A.resize(cols * rows, 1.0f);
    B.resize(cols * rows, 0.0f);
    nextA.resize(cols * rows, 1.0f);
    nextB.resize(cols * rows, 0.0f);

    presets = {
        {"Coral", 0.0545f, 0.062f},
        {"Spots", 0.035f, 0.065f},
        {"Stripes", 0.026f, 0.051f},
        {"Maze", 0.029f, 0.057f}};

    setParams(presets[presetIndex].f, presets[presetIndex].k);
    randomize();
}

int ReactionDiffusion::wrapX(int x) const
{
    if (x < 0)
        return x + cols;
    if (x >= cols)
        return x - cols;
    return x;
}

int ReactionDiffusion::wrapY(int y) const
{
    if (y < 0)
        return y + rows;
    if (y >= rows)
        return y - rows;
    return y;
}

int ReactionDiffusion::idx(int x, int y) const
{
    return wrapY(y) * cols + wrapX(x);
}

float ReactionDiffusion::laplace(const std::vector<float> &grid, int x, int y) const
{
    return -1.0f * grid[idx(x, y)] +
           0.25f * grid[idx(x + 1, y)] +
           0.25f * grid[idx(x - 1, y)] +
           0.25f * grid[idx(x, y + 1)] +
           0.25f * grid[idx(x, y - 1)];
}

void ReactionDiffusion::update(float dt)
{
    accumulator += dt;
    const float tick = 1.0f / tickRate;

    while (accumulator >= tick)
    {
        for (int y = 0; y < rows; y++)
        {
            for (int x = 0; x < cols; x++)
            {
                const int i = idx(x, y);
                const float laplaceA = laplace(A, x, y);
                const float laplaceB = laplace(B, x, y);
                const float reaction = A[i] * B[i] * B[i];

                nextA[i] = A[i] + (dA * laplaceA - reaction + f * (1.0f - A[i])) * simDt;
                nextB[i] = B[i] + (dB * laplaceB + reaction - (k + f) * B[i]) * simDt;

                nextA[i] = std::clamp(nextA[i], 0.0f, 1.0f);
                nextB[i] = std::clamp(nextB[i], 0.0f, 1.0f);
            }
        }

        A.swap(nextA);
        B.swap(nextB);
        accumulator -= tick;
    }
}

void ReactionDiffusion::draw(SDL_Renderer *renderer)
{
    SDL_SetRenderDrawColor(renderer, 12, 14, 18, 255);
    SDL_RenderClear(renderer);

    const SDL_Color p0 = {12, 14, 18, 255};
    const SDL_Color p1 = {20, 60, 80, 255};
    const SDL_Color p2 = {0, 180, 160, 255};
    const SDL_Color p3 = {255, 220, 80, 255};
    const SDL_Color p4 = {255, 255, 255, 255};

    for (int y = 0; y < rows; y++)
    {
        for (int x = 0; x < cols; x++)
        {
            const float v = std::clamp(B[y * cols + x], 0.0f, 1.0f);
            SDL_Color c;
            if (v < 0.3f)
                c = lerpColorRD(p0, p1, v / 0.3f);
            else if (v < 0.6f)
                c = lerpColorRD(p1, p2, (v - 0.3f) / 0.3f);
            else if (v < 0.8f)
                c = lerpColorRD(p2, p3, (v - 0.6f) / 0.2f);
            else
                c = lerpColorRD(p3, p4, (v - 0.8f) / 0.2f);

            SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, 255);
            SDL_Rect r = {x * cellSize, y * cellSize, cellSize, cellSize};
            SDL_RenderFillRect(renderer, &r);
        }
    }

    SDL_RenderPresent(renderer);
}

void ReactionDiffusion::clear()
{
    std::fill(A.begin(), A.end(), 1.0f);
    std::fill(B.begin(), B.end(), 0.0f);
    std::fill(nextA.begin(), nextA.end(), 1.0f);
    std::fill(nextB.begin(), nextB.end(), 0.0f);
    accumulator = 0.0f;
}

void ReactionDiffusion::randomize()
{
    clear();

    const int cx0 = cols / 2 - cols / 12;
    const int cx1 = cols / 2 + cols / 12;
    const int cy0 = rows / 2 - rows / 12;
    const int cy1 = rows / 2 + rows / 12;

    for (int y = cy0; y < cy1; y++)
    {
        for (int x = cx0; x < cx1; x++)
        {
            const int i = idx(x, y);
            const float seed = 0.5f + (static_cast<float>(rand()) / RAND_MAX) * 0.5f;
            B[i] = seed;
            A[i] = 1.0f - seed * 0.4f;
        }
    }
}

void ReactionDiffusion::injectAtPixel(int px, int py)
{
    if (px < 0 || py < 0 || px >= width || py >= height)
        return;

    const int cx = px / cellSize;
    const int cy = py / cellSize;
    const int brush = 5;

    for (int oy = -brush; oy <= brush; oy++)
    {
        for (int ox = -brush; ox <= brush; ox++)
        {
            const float d = std::sqrt(static_cast<float>(ox * ox + oy * oy));
            if (d > brush)
                continue;
            const int i = idx(cx + ox, cy + oy);
            const float amt = 1.0f - d / (brush + 0.01f);
            B[i] = std::clamp(B[i] + 0.25f * amt, 0.0f, 1.0f);
            A[i] = std::clamp(A[i] - 0.15f * amt, 0.0f, 1.0f);
        }
    }
}

void ReactionDiffusion::eraseAtPixel(int px, int py)
{
    if (px < 0 || py < 0 || px >= width || py >= height)
        return;

    const int cx = px / cellSize;
    const int cy = py / cellSize;
    const int brush = 5;

    for (int oy = -brush; oy <= brush; oy++)
    {
        for (int ox = -brush; ox <= brush; ox++)
        {
            const float d = std::sqrt(static_cast<float>(ox * ox + oy * oy));
            if (d > brush)
                continue;
            const int i = idx(cx + ox, cy + oy);
            const float amt = 1.0f - d / (brush + 0.01f);
            B[i] = std::clamp(B[i] - 0.35f * amt, 0.0f, 1.0f);
            A[i] = std::clamp(A[i] + 0.20f * amt, 0.0f, 1.0f);
        }
    }
}

void ReactionDiffusion::setParams(float fVal, float kVal)
{
    f = fVal;
    k = kVal;
}

void ReactionDiffusion::nextPreset()
{
    presetIndex = (presetIndex + 1) % static_cast<int>(presets.size());
    setParams(presets[presetIndex].f, presets[presetIndex].k);
    randomize();
}

std::string ReactionDiffusion::getPresetName() const
{
    return presets[presetIndex].name;
}
