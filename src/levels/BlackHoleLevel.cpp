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

BlackHoleLevel::BlackHoleLevel()
    : G(0.55f), softening(6.0f), timeScale(1.0f), jetTimer(0.0f)
{
    SimBody bh;
    bh.kind = BodyKind::BlackHole;
    bh.mass = 90000.0f;
    bh.radius = 22.0f;
    bh.color = {18, 18, 24, 255};
    bodies.push_back(bh);

    for (int i = 0; i < 1100; i++)
    {
        float a = (static_cast<float>(rand()) / RAND_MAX) * 6.2831853f;
        float r = 40.0f + (static_cast<float>(rand()) / RAND_MAX) * 320.0f;

        float v = std::sqrt((G * bh.mass) / (r + softening));
        AccretionParticle p;
        p.x = std::cos(a) * r;
        p.y = std::sin(a) * r;
        p.vx = -std::sin(a) * v;
        p.vy = std::cos(a) * v;
        p.temperature = std::clamp(1.0f - r / 360.0f, 0.05f, 1.0f);
        p.alive = true;
        disk.push_back(p);
    }
}

void BlackHoleLevel::update(float dt)
{
    float h = std::max(0.001f, dt * 60.0f * timeScale);
    jetTimer += dt * timeScale;

    for (size_t i = 0; i < disk.size(); i++)
    {
        if (!disk[i].alive)
            continue;

        float dx = -disk[i].x;
        float dy = -disk[i].y;
        float r2 = dx * dx + dy * dy + softening * softening;
        float r = std::sqrt(r2);
        float a = G * bodies[0].mass / r2;

        disk[i].vx += (dx / r) * a * h;
        disk[i].vy += (dy / r) * a * h;

        // frame dragging-like swirl intensification near the core
        float tx = -dy / std::max(1.0f, r);
        float ty = dx / std::max(1.0f, r);
        float swirl = std::clamp(0.12f * (1.0f - r / 280.0f), 0.0f, 0.12f);
        disk[i].vx += tx * swirl;
        disk[i].vy += ty * swirl;

        // accretion drag
        float drag = std::clamp(0.0035f * (1.0f + 220.0f / std::max(50.0f, r)), 0.0f, 0.09f);
        disk[i].vx *= (1.0f - drag);
        disk[i].vy *= (1.0f - drag);

        disk[i].x += disk[i].vx * h;
        disk[i].y += disk[i].vy * h;

        // temperature rises toward the hole
        disk[i].temperature = std::clamp(1.08f - r / 340.0f, 0.05f, 1.0f);

        if (r < 20.0f)
        {
            bodies[0].mass += 0.8f;
            bodies[0].radius = std::min(40.0f, std::sqrt(bodies[0].radius * bodies[0].radius + 0.5f));
            disk[i].alive = false;
        }
    }

    // Episodic bipolar jets eject hot particles perpendicular to disk.
    if (jetTimer > 0.10f)
    {
        jetTimer = 0.0f;
        for (int k = 0; k < 6; k++)
        {
            AccretionParticle j;
            j.x = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 8.0f;
            j.y = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 8.0f;
            j.vx = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.8f;
            const float dir = (rand() % 2 == 0) ? -1.0f : 1.0f;
            j.vy = dir * (4.5f + (static_cast<float>(rand()) / RAND_MAX) * 1.8f);
            j.temperature = 1.0f;
            j.alive = true;
            disk.push_back(j);
        }
    }

    disk.erase(std::remove_if(disk.begin(), disk.end(), [](const AccretionParticle &p)
                              {
                                  if (!p.alive)
                                      return true;
                                  return (std::abs(p.x) > 900.0f || std::abs(p.y) > 900.0f); }),
               disk.end());
}

void BlackHoleLevel::increasePopulation()
{
    for (int i = 0; i < 40; i++)
        spawnAt((static_cast<float>(rand()) / RAND_MAX - 0.5f) * 560.0f,
                (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 560.0f,
                false);
}

void BlackHoleLevel::decreasePopulation()
{
    int removed = 0;
    while (!disk.empty() && removed < 40)
    {
        disk.pop_back();
        removed++;
    }
}

int BlackHoleLevel::getBodyCount() const
{
    return 1 + static_cast<int>(disk.size());
}

void BlackHoleLevel::increaseTimeScale()
{
    timeScale = std::min(8.0f, timeScale + 0.1f);
}

void BlackHoleLevel::decreaseTimeScale()
{
    timeScale = std::max(0.2f, timeScale - 0.1f);
}

void BlackHoleLevel::draw(SDL_Renderer *renderer, const Camera &camera) const
{
    SDL_SetRenderDrawColor(renderer, 6, 6, 10, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);
    for (size_t i = 0; i < disk.size(); i++)
    {
        if (!disk[i].alive)
            continue;
        SDL_FPoint p = camera.worldToScreen(disk[i].x, disk[i].y);

        const float t = std::clamp(disk[i].temperature, 0.0f, 1.0f);
        const Uint8 rr = static_cast<Uint8>(255);
        const Uint8 gg = static_cast<Uint8>(90 + 150 * t);
        const Uint8 bb = static_cast<Uint8>(40 + 215 * t);
        const Uint8 aa = static_cast<Uint8>(40 + 130 * t);
        SDL_SetRenderDrawColor(renderer, rr, gg, bb, aa);
        SDL_RenderDrawPoint(renderer, static_cast<int>(p.x), static_cast<int>(p.y));
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_FPoint c = camera.worldToScreen(0.0f, 0.0f);
    SDL_SetRenderDrawColor(renderer, 80, 100, 160, 40);
    drawFilledCircle(renderer, static_cast<int>(c.x), static_cast<int>(c.y), 24);
    SDL_SetRenderDrawColor(renderer, 230, 170, 100, 30);
    drawFilledCircle(renderer, static_cast<int>(c.x), static_cast<int>(c.y), 36);
    SDL_SetRenderDrawColor(renderer, 10, 10, 14, 255);
    drawFilledCircle(renderer, static_cast<int>(c.x), static_cast<int>(c.y), 10);

    // jets
    SDL_SetRenderDrawColor(renderer, 120, 180, 255, 110);
    SDL_RenderDrawLine(renderer, static_cast<int>(c.x), static_cast<int>(c.y) - 6, static_cast<int>(c.x), static_cast<int>(c.y) - 150);
    SDL_RenderDrawLine(renderer, static_cast<int>(c.x), static_cast<int>(c.y) + 6, static_cast<int>(c.x), static_cast<int>(c.y) + 150);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    SDL_RenderPresent(renderer);
}

bool BlackHoleLevel::findBodyAtScreen(const Camera &camera, int sx, int sy, SimBody &outBody) const
{
    SDL_FPoint c = camera.worldToScreen(0.0f, 0.0f);
    float dx = c.x - sx;
    float dy = c.y - sy;
    if (dx * dx + dy * dy < 24.0f * 24.0f)
    {
        outBody = bodies[0];
        return true;
    }
    return false;
}

std::unique_ptr<Level> BlackHoleLevel::createChildFromBody(const SimBody &) const
{
    return nullptr;
}

void BlackHoleLevel::applyMouseGravity(float wx, float wy, float sign)
{
    for (size_t i = 0; i < disk.size(); i++)
    {
        if (!disk[i].alive)
            continue;
        float dx = wx - disk[i].x;
        float dy = wy - disk[i].y;
        float d2 = dx * dx + dy * dy;
        if (d2 < 2.0f)
            continue;
        float d = std::sqrt(d2);
        float f = sign * std::max(0.0f, 1.0f - d / 260.0f) * 1.6f;
        disk[i].vx += (dx / d) * f;
        disk[i].vy += (dy / d) * f;
    }
}

void BlackHoleLevel::spawnAt(float wx, float wy, bool)
{
    AccretionParticle p;
    p.x = wx;
    p.y = wy;
    p.vx = 0.0f;
    p.vy = 0.0f;
    p.temperature = 0.45f;
    p.alive = true;
    disk.push_back(p);
}
