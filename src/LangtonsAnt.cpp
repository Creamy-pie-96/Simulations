#include "LangtonsAnt.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>

static SDL_Color hsvToRgbLA(float h, float s, float v)
{
    float c = v * s;
    float x = c * (1.0f - std::abs(std::fmod(h / 60.0f, 2.0f) - 1.0f));
    float m = v - c;

    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;

    if (h < 60.0f)
    {
        r = c;
        g = x;
    }
    else if (h < 120.0f)
    {
        r = x;
        g = c;
    }
    else if (h < 180.0f)
    {
        g = c;
        b = x;
    }
    else if (h < 240.0f)
    {
        g = x;
        b = c;
    }
    else if (h < 300.0f)
    {
        r = x;
        b = c;
    }
    else
    {
        r = c;
        b = x;
    }

    return {
        static_cast<Uint8>((r + m) * 255.0f),
        static_cast<Uint8>((g + m) * 255.0f),
        static_cast<Uint8>((b + m) * 255.0f),
        255};
}

LangtonsAnt::LangtonsAnt(int width, int height, int cellSize)
    : width(width), height(height), cellSize(cellSize),
      tickRate(24.0f), accumulator(0.0f), stepCount(0)
{
    cols = width / cellSize;
    rows = height / cellSize;
    cells.resize(cols * rows, 0);
    addAnt();
}

int LangtonsAnt::wrapX(int x) const
{
    if (x < 0)
        return x + cols;
    if (x >= cols)
        return x - cols;
    return x;
}

int LangtonsAnt::wrapY(int y) const
{
    if (y < 0)
        return y + rows;
    if (y >= rows)
        return y - rows;
    return y;
}

int LangtonsAnt::idx(int x, int y) const
{
    return wrapY(y) * cols + wrapX(x);
}

void LangtonsAnt::rebuildAntColors()
{
    antColors.clear();
    const int n = std::max(1, static_cast<int>(ants.size()));
    for (int i = 0; i < n; i++)
    {
        float h = (360.0f / n) * i;
        antColors.push_back(hsvToRgbLA(h, 0.85f, 1.0f));
    }
}

void LangtonsAnt::addAnt()
{
    if (ants.size() >= 24)
        return;

    Ant a;
    a.x = rand() % cols;
    a.y = rand() % rows;
    a.dir = rand() % 4;
    a.type = static_cast<int>(ants.size());
    ants.push_back(a);
    rebuildAntColors();
}

void LangtonsAnt::addAntAtPixel(int px, int py)
{
    if (ants.size() >= 24)
        return;
    if (px < 0 || py < 0 || px >= width || py >= height)
        return;

    Ant a;
    a.x = wrapX(px / cellSize);
    a.y = wrapY(py / cellSize);
    a.dir = rand() % 4;
    a.type = static_cast<int>(ants.size());
    ants.push_back(a);
    rebuildAntColors();
}

void LangtonsAnt::paintCell(int px, int py, bool black)
{
    if (px < 0 || py < 0 || px >= width || py >= height)
        return;
    const int gx = px / cellSize;
    const int gy = py / cellSize;
    cells[idx(gx, gy)] = black ? 1 : 0;
}

void LangtonsAnt::step()
{
    for (auto &a : ants)
    {
        const int i = idx(a.x, a.y);
        if (cells[i] == 0)
        {
            a.dir = (a.dir + 1) % 4;
            cells[i] = 1;
        }
        else
        {
            a.dir = (a.dir + 3) % 4;
            cells[i] = 0;
        }

        if (a.dir == 0)
            a.y = wrapY(a.y - 1);
        else if (a.dir == 1)
            a.x = wrapX(a.x + 1);
        else if (a.dir == 2)
            a.y = wrapY(a.y + 1);
        else
            a.x = wrapX(a.x - 1);

        stepCount++;
    }
}

void LangtonsAnt::update(float dt)
{
    accumulator += dt;
    const float tick = 1.0f / tickRate;

    while (accumulator >= tick)
    {
        step();
        accumulator -= tick;
    }
}

void LangtonsAnt::draw(SDL_Renderer *renderer)
{
    SDL_SetRenderDrawColor(renderer, 240, 240, 220, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 30, 30, 40, 255);
    for (int y = 0; y < rows; y++)
    {
        for (int x = 0; x < cols; x++)
        {
            if (cells[y * cols + x] == 0)
                continue;

            SDL_Rect r = {x * cellSize, y * cellSize, cellSize, cellSize};
            SDL_RenderFillRect(renderer, &r);
        }
    }

    for (const auto &a : ants)
    {
        const SDL_Color col = antColors[a.type % static_cast<int>(antColors.size())];
        SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, 255);

        const int px = a.x * cellSize + cellSize / 2;
        const int py = a.y * cellSize + cellSize / 2;
        SDL_Rect antRect = {px - 1, py - 1, 3, 3};
        SDL_RenderFillRect(renderer, &antRect);
    }

    SDL_RenderPresent(renderer);
}

void LangtonsAnt::clear()
{
    std::fill(cells.begin(), cells.end(), 0);
    stepCount = 0;
    accumulator = 0.0f;
}

void LangtonsAnt::randomize()
{
    for (auto &c : cells)
        c = (rand() % 100 < 20) ? 1 : 0;

    for (auto &a : ants)
    {
        a.x = rand() % cols;
        a.y = rand() % rows;
        a.dir = rand() % 4;
    }

    stepCount = 0;
    accumulator = 0.0f;
}

void LangtonsAnt::increaseSpeed()
{
    tickRate = std::min(120.0f, tickRate + 2.0f);
}

void LangtonsAnt::decreaseSpeed()
{
    tickRate = std::max(1.0f, tickRate - 2.0f);
}
