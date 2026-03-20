#include "levels/GalaxyMapLevel.hpp"

#include "levels/GalaxyLevel.hpp"

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

GalaxyMapLevel::GalaxyMapLevel()
    : G(0.08f), coreMass(5.0e8f), timeScale(1.0f)
{
    const int clusters = 8;
    for (int i = 0; i < clusters; i++)
    {
        const float a = (static_cast<float>(rand()) / RAND_MAX) * 6.2831853f;
        const float r = 2400.0f + (static_cast<float>(rand()) / RAND_MAX) * 7800.0f;

        SimBody c;
        c.kind = BodyKind::Cluster;
        c.x = std::cos(a) * r;
        c.y = std::sin(a) * r;
        c.mass = 4.0e7f + (static_cast<float>(rand()) / RAND_MAX) * 1.8e8f;
        c.radius = 140.0f;
        c.color = {120, 170, 255, 255};

        const float v = std::sqrt((G * coreMass) / std::max(200.0f, r));
        c.vx = -std::sin(a) * v;
        c.vy = std::cos(a) * v;
        bodies.push_back(c);
    }
}

void GalaxyMapLevel::update(float dt)
{
    const float h = std::max(0.001f, dt * 60.0f * timeScale);
    for (auto &b : bodies)
    {
        float dx = -b.x;
        float dy = -b.y;
        float r2 = dx * dx + dy * dy + 220.0f * 220.0f;
        float r = std::sqrt(r2);
        float a = (G * coreMass) / r2;
        b.vx += (dx / r) * a * h;
        b.vy += (dy / r) * a * h;
        b.x += b.vx * h;
        b.y += b.vy * h;
    }
}

void GalaxyMapLevel::increasePopulation()
{
    spawnAt((static_cast<float>(rand()) / RAND_MAX - 0.5f) * 8000.0f,
            (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 8000.0f,
            false);
}

void GalaxyMapLevel::decreasePopulation()
{
    if (!bodies.empty())
        bodies.pop_back();
}

void GalaxyMapLevel::increaseTimeScale()
{
    timeScale = std::min(8.0f, timeScale + 0.1f);
}

void GalaxyMapLevel::decreaseTimeScale()
{
    timeScale = std::max(0.2f, timeScale - 0.1f);
}

void GalaxyMapLevel::draw(SDL_Renderer *renderer, const Camera &camera) const
{
    SDL_SetRenderDrawColor(renderer, 3, 5, 9, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    for (const auto &b : bodies)
    {
        SDL_FPoint p = camera.worldToScreen(b.x, b.y);
        SDL_SetRenderDrawColor(renderer, b.color.r, b.color.g, b.color.b, 45);
        drawFilledCircle(renderer, static_cast<int>(p.x), static_cast<int>(p.y), 8);
        SDL_SetRenderDrawColor(renderer, b.color.r, b.color.g, b.color.b, 255);
        drawFilledCircle(renderer, static_cast<int>(p.x), static_cast<int>(p.y), 3);
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    SDL_RenderPresent(renderer);
}

bool GalaxyMapLevel::findBodyAtScreen(const Camera &camera, int sx, int sy, SimBody &outBody) const
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

std::unique_ptr<Level> GalaxyMapLevel::createChildFromBody(const SimBody &body) const
{
    if (body.kind == BodyKind::Cluster)
        return std::make_unique<GalaxyLevel>(body.mass);
    return nullptr;
}

void GalaxyMapLevel::applyMouseGravity(float wx, float wy, float sign)
{
    for (auto &b : bodies)
    {
        float dx = wx - b.x;
        float dy = wy - b.y;
        float d2 = dx * dx + dy * dy;
        if (d2 < 4.0f)
            continue;
        float d = std::sqrt(d2);
        float f = sign * 12.0f / (1.0f + d * 0.012f);
        b.vx += (dx / d) * f / std::max(1.0f, b.mass) * 30000.0f;
        b.vy += (dy / d) * f / std::max(1.0f, b.mass) * 30000.0f;
    }
}

void GalaxyMapLevel::spawnAt(float wx, float wy, bool blackHole)
{
    SimBody b;
    b.x = wx;
    b.y = wy;
    b.vx = 0.0f;
    b.vy = 0.0f;
    b.alive = true;
    if (blackHole)
    {
        b.kind = BodyKind::BlackHole;
        b.mass = 2.2e8f;
        b.radius = 180.0f;
        b.color = {40, 40, 50, 255};
    }
    else
    {
        b.kind = BodyKind::Cluster;
        b.mass = 7.0e7f;
        b.radius = 120.0f;
        b.color = {140, 190, 255, 255};
    }
    bodies.push_back(b);
}
