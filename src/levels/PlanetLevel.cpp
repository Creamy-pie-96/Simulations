#include "levels/PlanetLevel.hpp"

#include "levels/BlackHoleLevel.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace
{
    void drawFilledCircle(SDL_Renderer *renderer, int cx, int cy, int radius)
    {
        for (int y = -radius; y <= radius; y++)
        {
            int xx = static_cast<int>(std::sqrt(radius * radius - y * y));
            SDL_RenderDrawLine(renderer, cx - xx, cy + y, cx + xx, cy + y);
        }
    }
}

PlanetLevel::PlanetLevel(float planetMass)
    : G(0.26f), softening(8.0f), timeScale(1.0f)
{
    SimBody p;
    p.kind = BodyKind::Planet;
    p.mass = std::max(450.0f, planetMass);
    p.radius = 20.0f;
    p.color = {120, 180, 255, 255};
    bodies.push_back(p);

    int moons = 3 + rand() % 6;
    for (int i = 0; i < moons; i++)
    {
        float r = 70.0f + i * 55.0f + (rand() % 20);
        float a = (static_cast<float>(rand()) / RAND_MAX) * 6.2831853f;

        SimBody m;
        m.kind = BodyKind::Moon;
        m.x = std::cos(a) * r;
        m.y = std::sin(a) * r;
        m.mass = 6.0f + (static_cast<float>(rand()) / RAND_MAX) * 20.0f;
        m.radius = 5.0f + (static_cast<float>(rand()) / RAND_MAX) * 4.0f;
        m.color = {200, 210, 230, 255};

        float v = std::sqrt((G * p.mass) / (r + softening));
        m.vx = -std::sin(a) * v;
        m.vy = std::cos(a) * v;
        bodies.push_back(m);
    }
}

void PlanetLevel::update(float dt)
{
    float h = std::max(0.001f, dt * 60.0f * timeScale);
    for (size_t i = 1; i < bodies.size(); i++)
    {
        float dx = -bodies[i].x;
        float dy = -bodies[i].y;
        float r2 = dx * dx + dy * dy + softening * softening;
        float r = std::sqrt(r2);
        float a = G * bodies[0].mass / r2;
        bodies[i].vx += (dx / r) * a * h;
        bodies[i].vy += (dy / r) * a * h;
        bodies[i].x += bodies[i].vx * h;
        bodies[i].y += bodies[i].vy * h;
    }
}

void PlanetLevel::increasePopulation()
{
    spawnAt((static_cast<float>(rand()) / RAND_MAX - 0.5f) * 260.0f,
            (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 260.0f,
            false);
}

void PlanetLevel::decreasePopulation()
{
    if (bodies.size() > 1)
        bodies.pop_back();
}

void PlanetLevel::increaseTimeScale()
{
    timeScale = std::min(8.0f, timeScale + 0.1f);
}

void PlanetLevel::decreaseTimeScale()
{
    timeScale = std::max(0.2f, timeScale - 0.1f);
}

void PlanetLevel::draw(SDL_Renderer *renderer, const Camera &camera) const
{
    SDL_SetRenderDrawColor(renderer, 8, 10, 16, 255);
    SDL_RenderClear(renderer);

    SDL_FPoint cp = camera.worldToScreen(0.0f, 0.0f);
    SDL_SetRenderDrawColor(renderer, 120, 180, 255, 255);
    drawFilledCircle(renderer, static_cast<int>(cp.x), static_cast<int>(cp.y), 16);

    for (size_t i = 1; i < bodies.size(); i++)
    {
        SDL_FPoint p = camera.worldToScreen(bodies[i].x, bodies[i].y);
        SDL_SetRenderDrawColor(renderer, bodies[i].color.r, bodies[i].color.g, bodies[i].color.b, 255);
        drawFilledCircle(renderer, static_cast<int>(p.x), static_cast<int>(p.y), 3);
    }

    SDL_RenderPresent(renderer);
}

bool PlanetLevel::findBodyAtScreen(const Camera &camera, int sx, int sy, SimBody &outBody) const
{
    float best = 1.0e9f;
    bool found = false;
    for (const auto &b : bodies)
    {
        SDL_FPoint p = camera.worldToScreen(b.x, b.y);
        float dx = p.x - sx;
        float dy = p.y - sy;
        float d2 = dx * dx + dy * dy;
        if (d2 < best && d2 < 20.0f * 20.0f)
        {
            best = d2;
            outBody = b;
            found = true;
        }
    }
    return found;
}

std::unique_ptr<Level> PlanetLevel::createChildFromBody(const SimBody &body) const
{
    if (body.kind == BodyKind::BlackHole)
        return std::make_unique<BlackHoleLevel>();
    return nullptr;
}

void PlanetLevel::applyMouseGravity(float wx, float wy, float sign)
{
    for (size_t i = 1; i < bodies.size(); i++)
    {
        float dx = wx - bodies[i].x;
        float dy = wy - bodies[i].y;
        float d2 = dx * dx + dy * dy;
        if (d2 < 2.0f)
            continue;
        float d = std::sqrt(d2);
        float f = sign * std::max(0.0f, 1.0f - d / 220.0f) * 0.35f;
        bodies[i].vx += (dx / d) * f;
        bodies[i].vy += (dy / d) * f;
    }
}

void PlanetLevel::spawnAt(float wx, float wy, bool blackHole)
{
    SimBody b;
    b.x = wx;
    b.y = wy;
    if (blackHole)
    {
        b.kind = BodyKind::BlackHole;
        b.mass = 9000.0f;
        b.radius = 12.0f;
        b.color = {20, 20, 25, 255};
    }
    else
    {
        b.kind = BodyKind::Moon;
        b.mass = 10.0f;
        b.radius = 6.0f;
        b.color = {220, 220, 230, 255};
    }
    bodies.push_back(b);
}
