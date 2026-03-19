#include "GameOfLife.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>

static SDL_Color lerpColorGOL(const SDL_Color &a, const SDL_Color &b, float t)
{
    t = std::clamp(t, 0.0f, 1.0f);
    SDL_Color c;
    c.r = static_cast<Uint8>(a.r + (b.r - a.r) * t);
    c.g = static_cast<Uint8>(a.g + (b.g - a.g) * t);
    c.b = static_cast<Uint8>(a.b + (b.b - a.b) * t);
    c.a = 255;
    return c;
}

static SDL_Color colorForCellValue(float v)
{
    const SDL_Color bornA = {255, 255, 200, 255};
    const SDL_Color bornB = {255, 220, 50, 255};
    const SDL_Color youngA = {255, 220, 50, 255};
    const SDL_Color youngB = {80, 255, 120, 255};
    const SDL_Color matureA = {80, 255, 120, 255};
    const SDL_Color matureB = {20, 180, 80, 255};
    const SDL_Color oldA = {20, 180, 80, 255};
    const SDL_Color oldB = {0, 120, 100, 255};
    const SDL_Color dyingA = {20, 80, 40, 255};
    const SDL_Color dyingB = {5, 15, 8, 255};

    if (v > 0.0f)
    {
        if (v <= 3.0f)
            return lerpColorGOL(bornA, bornB, (v - 1.0f) / 2.0f);
        if (v <= 15.0f)
            return lerpColorGOL(youngA, youngB, (v - 4.0f) / 11.0f);
        if (v <= 40.0f)
            return lerpColorGOL(matureA, matureB, (v - 16.0f) / 24.0f);
        return lerpColorGOL(oldA, oldB, (v - 41.0f) / 59.0f);
    }

    if (v < 0.0f)
    {
        return lerpColorGOL(dyingA, dyingB, (v + 10.0f) / 9.0f);
    }

    return {0, 0, 0, 255};
}

GameOfLife::GameOfLife(int width, int height, int cellSize)
    : width(width), height(height), cellSize(cellSize), tickRate(12.0f), accumulator(0.0f)
{
    cols = width / cellSize;
    rows = height / cellSize;
    cells.resize(cols * rows, 0);
    next.resize(cols * rows, 0);
}

int GameOfLife::wrapX(int x) const
{
    if (x < 0)
        return x + cols;
    if (x >= cols)
        return x - cols;
    return x;
}

int GameOfLife::wrapY(int y) const
{
    if (y < 0)
        return y + rows;
    if (y >= rows)
        return y - rows;
    return y;
}

int GameOfLife::idx(int x, int y) const
{
    return wrapY(y) * cols + wrapX(x);
}

int GameOfLife::countNeighbors(int x, int y) const
{
    int n = 0;
    for (int oy = -1; oy <= 1; oy++)
    {
        for (int ox = -1; ox <= 1; ox++)
        {
            if (ox == 0 && oy == 0)
                continue;
            n += (cells[idx(x + ox, y + oy)] > 0.0f) ? 1 : 0;
        }
    }
    return n;
}

void GameOfLife::step()
{
    for (int y = 0; y < rows; y++)
    {
        for (int x = 0; x < cols; x++)
        {
            const int i = y * cols + x;
            const int neighbors = countNeighbors(x, y);
            const bool alive = cells[i] > 0.0f;

            if (alive)
            {
                if (neighbors == 2 || neighbors == 3)
                    next[i] = std::min(100.0f, cells[i] + 1.0f);
                else
                    next[i] = -10.0f;
            }
            else
            {
                if (neighbors == 3)
                    next[i] = 1.0f;
                else if (cells[i] < 0.0f)
                    next[i] = std::min(0.0f, cells[i] + 1.0f);
                else
                    next[i] = 0.0f;
            }
        }
    }

    cells.swap(next);
}

void GameOfLife::update(float dt)
{
    const float fadeStep = std::max(0.0f, dt * 60.0f);
    for (auto &c : cells)
    {
        if (c < 0.0f)
            c = std::min(0.0f, c + fadeStep);
    }

    accumulator += dt;
    const float tick = 1.0f / tickRate;

    while (accumulator >= tick)
    {
        step();
        accumulator -= tick;
    }
}

void GameOfLife::draw(SDL_Renderer *renderer) const
{
    SDL_SetRenderDrawColor(renderer, 12, 14, 18, 255);
    SDL_RenderClear(renderer);

    for (int y = 0; y < rows; y++)
    {
        for (int x = 0; x < cols; x++)
        {
            const float v = cells[y * cols + x];
            if (std::abs(v) < 0.0001f)
                continue;

            SDL_Color c = colorForCellValue(v);
            SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, 255);
            SDL_Rect r = {x * cellSize, y * cellSize, cellSize - 1, cellSize - 1};
            SDL_RenderFillRect(renderer, &r);
        }
    }

    if (cellSize >= 7)
    {
        SDL_SetRenderDrawColor(renderer, 24, 28, 36, 255);
        for (int x = 0; x <= cols; x++)
            SDL_RenderDrawLine(renderer, x * cellSize, 0, x * cellSize, rows * cellSize);
        for (int y = 0; y <= rows; y++)
            SDL_RenderDrawLine(renderer, 0, y * cellSize, cols * cellSize, y * cellSize);
    }

    SDL_RenderPresent(renderer);
}

void GameOfLife::clear()
{
    std::fill(cells.begin(), cells.end(), 0);
    std::fill(next.begin(), next.end(), 0);
}

void GameOfLife::randomize(float aliveProbability)
{
    for (auto &c : cells)
    {
        float r = static_cast<float>(rand()) / RAND_MAX;
        c = (r < aliveProbability) ? 1.0f : 0.0f;
    }
}

void GameOfLife::paintCell(int x, int y, bool alive)
{
    if (x < 0 || y < 0 || x >= width || y >= height)
        return;

    const int gx = x / cellSize;
    const int gy = y / cellSize;
    cells[idx(gx, gy)] = alive ? 1.0f : 0.0f;
}

void GameOfLife::placePattern(const std::string &name, int cx, int cy)
{
    const int gx = std::clamp(cx / cellSize, 0, cols - 1);
    const int gy = std::clamp(cy / cellSize, 0, rows - 1);

    std::vector<std::pair<int, int>> pattern;

    if (name == "glider")
    {
        pattern = {{1, 0}, {2, 1}, {0, 2}, {1, 2}, {2, 2}};
    }
    else if (name == "pulsar")
    {
        pattern = {
            {-4, -6}, {-3, -6}, {-2, -6}, {2, -6}, {3, -6}, {4, -6}, {-6, -4}, {-1, -4}, {1, -4}, {6, -4}, {-6, -3}, {-1, -3}, {1, -3}, {6, -3}, {-6, -2}, {-1, -2}, {1, -2}, {6, -2}, {-4, -1}, {-3, -1}, {-2, -1}, {2, -1}, {3, -1}, {4, -1}, {-4, 1}, {-3, 1}, {-2, 1}, {2, 1}, {3, 1}, {4, 1}, {-6, 2}, {-1, 2}, {1, 2}, {6, 2}, {-6, 3}, {-1, 3}, {1, 3}, {6, 3}, {-6, 4}, {-1, 4}, {1, 4}, {6, 4}, {-4, 6}, {-3, 6}, {-2, 6}, {2, 6}, {3, 6}, {4, 6}};
    }
    else if (name == "gosper")
    {
        pattern = {
            {0, 4}, {0, 5}, {1, 4}, {1, 5}, {10, 4}, {10, 5}, {10, 6}, {11, 3}, {11, 7}, {12, 2}, {12, 8}, {13, 2}, {13, 8}, {14, 5}, {15, 3}, {15, 7}, {16, 4}, {16, 5}, {16, 6}, {17, 5}, {20, 2}, {20, 3}, {20, 4}, {21, 2}, {21, 3}, {21, 4}, {22, 1}, {22, 5}, {24, 0}, {24, 1}, {24, 5}, {24, 6}, {34, 2}, {34, 3}, {35, 2}, {35, 3}};
    }
    else
    {
        return;
    }

    for (const auto &[ox, oy] : pattern)
    {
        cells[idx(gx + ox, gy + oy)] = 1.0f;
    }
}

void GameOfLife::increaseTickRate()
{
    tickRate = std::min(60.0f, tickRate + 1.0f);
}

void GameOfLife::decreaseTickRate()
{
    tickRate = std::max(1.0f, tickRate - 1.0f);
}

int GameOfLife::getAliveCount() const
{
    int alive = 0;
    for (auto c : cells)
        alive += (c > 0.0f) ? 1 : 0;
    return alive;
}

int GameOfLife::getFadingCount() const
{
    int fading = 0;
    for (auto c : cells)
        fading += (c < 0.0f) ? 1 : 0;
    return fading;
}
