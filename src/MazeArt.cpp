#include "MazeArt.hpp"

#include <SDL2/SDL_image.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace
{
    float darknessWeight(Uint8 r, Uint8 g, Uint8 b)
    {
        float brightness = (r + g + b) / 765.0f;
        return 1.0f - brightness + 0.05f;
    }
}

MazeArt::MazeArt(int width, int height, int cell)
    : width(width), height(height), cellSize(std::max(6, cell)),
      cols(width / std::max(6, cell)), rows(height / std::max(6, cell)),
      stepsPerFrame(40.0f), stepAccumulator(0.0f), visitedCount(0), imageLoaded(false), monochrome(false)
{
    regenerate(true);
}

void MazeArt::seedRandomColors()
{
    imageMap.assign(cols * rows, Pixel{});
    for (auto &p : imageMap)
    {
        p.r = static_cast<Uint8>(80 + rand() % 170);
        p.g = static_cast<Uint8>(80 + rand() % 170);
        p.b = static_cast<Uint8>(80 + rand() % 170);
    }
}

void MazeArt::regenerate(bool clearImageWeights)
{
    grid.assign(cols * rows, Cell{});
    stack.clear();
    visitedCount = 0;

    if (clearImageWeights || !imageLoaded)
    {
        imageLoaded = false;
        imagePath.clear();
        seedRandomColors();
    }

    int sx = rand() % cols;
    int sy = rand() % rows;
    int s = idx(sx, sy);
    grid[s].visited = true;
    const Pixel &p = imageMap[s];
    grid[s].color = {p.r, p.g, p.b, 255};
    visitedCount = 1;
    stack.push_back(s);
}

bool MazeArt::loadImage(const std::string &path)
{
    SDL_Surface *surf = IMG_Load(path.c_str());
    if (!surf)
        surf = SDL_LoadBMP(path.c_str());
    if (!surf)
        return false;

    SDL_Surface *conv = SDL_ConvertSurfaceFormat(surf, SDL_PIXELFORMAT_RGB24, 0);
    SDL_FreeSurface(surf);
    if (!conv)
        return false;

    imageMap.assign(cols * rows, Pixel{});
    for (int y = 0; y < rows; y++)
    {
        for (int x = 0; x < cols; x++)
        {
            int sx = x * conv->w / cols;
            int sy = y * conv->h / rows;
            Uint8 *pix = static_cast<Uint8 *>(conv->pixels) + sy * conv->pitch + sx * 3;

            Pixel p;
            p.r = pix[0];
            p.g = pix[1];
            p.b = pix[2];
            imageMap[idx(x, y)] = p;
        }
    }

    SDL_FreeSurface(conv);

    imageLoaded = true;
    imagePath = path;
    regenerate(false);
    return true;
}

void MazeArt::unloadImage()
{
    imageLoaded = false;
    imagePath.clear();
    seedRandomColors();
    regenerate(false);
}

int MazeArt::pickWeightedNeighbor(int cx, int cy, const std::vector<int> &neighbors) const
{
    if (neighbors.empty())
        return -1;

    if (!imageLoaded)
    {
        return neighbors[rand() % static_cast<int>(neighbors.size())];
    }

    float total = 0.0f;
    std::vector<float> ws;
    ws.reserve(neighbors.size());
    for (int n : neighbors)
    {
        int nx = n % cols;
        int ny = n / cols;
        const Pixel &p = imageMap[idx(nx, ny)];
        float w = darknessWeight(p.r, p.g, p.b);
        ws.push_back(w);
        total += w;
    }

    float r = (static_cast<float>(rand()) / RAND_MAX) * total;
    for (size_t i = 0; i < neighbors.size(); i++)
    {
        r -= ws[i];
        if (r <= 0.0f)
            return neighbors[i];
    }
    return neighbors.back();
}

void MazeArt::carveStep()
{
    if (stack.empty())
        return;

    int c = stack.back();
    int cx = c % cols;
    int cy = c / cols;

    std::vector<int> n;
    n.reserve(4);

    // N E S W
    const int nx[4] = {cx, cx + 1, cx, cx - 1};
    const int ny[4] = {cy - 1, cy, cy + 1, cy};

    for (int d = 0; d < 4; d++)
    {
        if (!inBounds(nx[d], ny[d]))
            continue;
        int ni = idx(nx[d], ny[d]);
        if (!grid[ni].visited)
            n.push_back(ni);
    }

    if (n.empty())
    {
        stack.pop_back();
        return;
    }

    int ni = pickWeightedNeighbor(cx, cy, n);
    int nxv = ni % cols;
    int nyv = ni / cols;

    // remove wall between current and next
    if (nyv == cy - 1)
    {
        grid[c].walls[0] = false;
        grid[ni].walls[2] = false;
    }
    else if (nxv == cx + 1)
    {
        grid[c].walls[1] = false;
        grid[ni].walls[3] = false;
    }
    else if (nyv == cy + 1)
    {
        grid[c].walls[2] = false;
        grid[ni].walls[0] = false;
    }
    else if (nxv == cx - 1)
    {
        grid[c].walls[3] = false;
        grid[ni].walls[1] = false;
    }

    grid[ni].visited = true;
    const Pixel &p = imageMap[ni];
    grid[ni].color = {p.r, p.g, p.b, 255};
    visitedCount++;
    stack.push_back(ni);
}

void MazeArt::update(float)
{
    stepAccumulator += stepsPerFrame;
    int steps = static_cast<int>(stepAccumulator);
    stepAccumulator -= steps;

    if (steps <= 0)
        return;

    for (int i = 0; i < steps; i++)
        carveStep();
}

void MazeArt::draw(SDL_Renderer *renderer)
{
    SDL_SetRenderDrawColor(renderer, 10, 10, 14, 255);
    SDL_RenderClear(renderer);

    // draw carved corridors as lines between cell centers
    for (int y = 0; y < rows; y++)
    {
        for (int x = 0; x < cols; x++)
        {
            const Cell &c = grid[idx(x, y)];
            if (!c.visited)
                continue;

            int cx = x * cellSize + cellSize / 2;
            int cy = y * cellSize + cellSize / 2;

            SDL_Color col = c.color;
            if (monochrome)
            {
                Uint8 g = static_cast<Uint8>((static_cast<int>(col.r) + col.g + col.b) / 3);
                col = {g, g, g, 255};
            }
            SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, 255);

            if (!c.walls[0] && y > 0)
            {
                SDL_RenderDrawLine(renderer, cx, cy, cx, (y - 1) * cellSize + cellSize / 2);
            }
            if (!c.walls[1] && x < cols - 1)
            {
                SDL_RenderDrawLine(renderer, cx, cy, (x + 1) * cellSize + cellSize / 2, cy);
            }
            if (!c.walls[2] && y < rows - 1)
            {
                SDL_RenderDrawLine(renderer, cx, cy, cx, (y + 1) * cellSize + cellSize / 2);
            }
            if (!c.walls[3] && x > 0)
            {
                SDL_RenderDrawLine(renderer, cx, cy, (x - 1) * cellSize + cellSize / 2, cy);
            }

            SDL_Rect dot{cx - 1, cy - 1, 2, 2};
            SDL_RenderFillRect(renderer, &dot);
        }
    }

    // active backtracker head + tail
    if (!stack.empty())
    {
        int h = stack.back();
        int hx = h % cols;
        int hy = h / cols;
        int cx = hx * cellSize + cellSize / 2;
        int cy = hy * cellSize + cellSize / 2;

        SDL_SetRenderDrawColor(renderer, 70, 255, 120, 80);
        int tailStart = std::max(0, static_cast<int>(stack.size()) - 60);
        for (int i = static_cast<int>(stack.size()) - 1; i > tailStart; --i)
        {
            int a = stack[i];
            int b = stack[i - 1];
            int ax = (a % cols) * cellSize + cellSize / 2;
            int ay = (a / cols) * cellSize + cellSize / 2;
            int bx = (b % cols) * cellSize + cellSize / 2;
            int by = (b / cols) * cellSize + cellSize / 2;
            SDL_RenderDrawLine(renderer, ax, ay, bx, by);
        }

        SDL_Rect r{cx - 2, cy - 2, 4, 4};
        SDL_SetRenderDrawColor(renderer, 70, 255, 120, 255);
        SDL_RenderFillRect(renderer, &r);
    }
    else
    {
        // idle marker (white)
        SDL_Rect r{width / 2 - 2, height / 2 - 2, 4, 4};
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderFillRect(renderer, &r);
    }

    SDL_RenderPresent(renderer);
}

void MazeArt::increaseSpeed()
{
    if (stepsPerFrame < 1.0f)
        stepsPerFrame = std::min(1.0f, stepsPerFrame + 0.1f);
    else
        stepsPerFrame = std::min(800.0f, stepsPerFrame * 1.15f);
}

void MazeArt::decreaseSpeed()
{
    if (stepsPerFrame <= 1.0f)
        stepsPerFrame = std::max(0.1f, stepsPerFrame - 0.1f);
    else
        stepsPerFrame = std::max(0.1f, stepsPerFrame * 0.85f);
}

void MazeArt::toggleMonochrome()
{
    monochrome = !monochrome;
}
