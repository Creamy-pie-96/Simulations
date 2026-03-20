#include "levels/SolarSystemLevel.hpp"

#include "levels/BlackHoleLevel.hpp"
#include "levels/PlanetLevel.hpp"

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

SolarSystemLevel::SolarSystemLevel(float starMass)
    : G(0.18f), softening(12.0f), timeScale(1.0f)
{
    SimBody star;
    star.kind = BodyKind::Star;
    star.mass = std::max(40000.0f, starMass * 1.6f);
    star.radius = 42.0f;
    star.color = {255, 220, 120, 255};
    bodies.push_back(star);

    int planets = 5 + rand() % 6;
    for (int i = 0; i < planets; i++)
    {
        float r = 120.0f * std::pow(1.65f, static_cast<float>(i)) + (rand() % 80);
        float a = (static_cast<float>(rand()) / RAND_MAX) * 6.2831853f;

        SimBody p;
        p.kind = BodyKind::Planet;
        p.x = std::cos(a) * r;
        p.y = std::sin(a) * r;
        p.mass = 20.0f + (static_cast<float>(rand()) / RAND_MAX) * 110.0f;
        p.radius = 9.0f + (static_cast<float>(rand()) / RAND_MAX) * 8.0f;
        p.color = {static_cast<Uint8>(100 + rand() % 120), static_cast<Uint8>(120 + rand() % 100), static_cast<Uint8>(140 + rand() % 100), 255};

        float v = std::sqrt((G * star.mass) / (r + softening));
        p.vx = -std::sin(a) * v;
        p.vy = std::cos(a) * v;
        bodies.push_back(p);
        const int parentIdx = static_cast<int>(bodies.size()) - 1;

        const int moons = 1 + rand() % 3;
        for (int m = 0; m < moons; m++)
        {
            const float mr = 20.0f + m * 10.0f + (rand() % 8);
            const float ma = (static_cast<float>(rand()) / RAND_MAX) * 6.2831853f;

            SimBody moon;
            moon.kind = BodyKind::Moon;
            moon.mass = 4.0f + (static_cast<float>(rand()) / RAND_MAX) * 12.0f;
            moon.radius = 4.0f + (static_cast<float>(rand()) / RAND_MAX) * 3.0f;
            moon.color = {210, 215, 230, 255};

            moon.x = p.x + std::cos(ma) * mr;
            moon.y = p.y + std::sin(ma) * mr;

            const float mv = std::sqrt((G * p.mass) / (mr + 2.0f));
            moon.vx = p.vx - std::sin(ma) * mv;
            moon.vy = p.vy + std::cos(ma) * mv;
            moon.parent = parentIdx;
            bodies.push_back(moon);
        }
    }

    // Set star velocity so total momentum is near zero (stable barycentric frame).
    if (!bodies.empty())
    {
        float px = 0.0f;
        float py = 0.0f;
        for (size_t i = 1; i < bodies.size(); i++)
        {
            px += bodies[i].mass * bodies[i].vx;
            py += bodies[i].mass * bodies[i].vy;
        }
        bodies[0].vx = -px / std::max(1.0f, bodies[0].mass);
        bodies[0].vy = -py / std::max(1.0f, bodies[0].mass);
    }
}

void SolarSystemLevel::update(float dt)
{
    const float dtFrame = std::clamp(dt * 60.0f * timeScale, 0.03f, 1.6f);
    const int substeps = (dtFrame > 1.0f) ? 3 : ((dtFrame > 0.5f) ? 2 : 1);
    const float h = dtFrame / static_cast<float>(substeps);

    auto computeAcc = [&](std::vector<float> &ax, std::vector<float> &ay)
    {
        ax.assign(bodies.size(), 0.0f);
        ay.assign(bodies.size(), 0.0f);

        const float eps2 = softening * softening;
        for (size_t i = 0; i < bodies.size(); i++)
        {
            if (!bodies[i].alive)
                continue;

            for (size_t j = i + 1; j < bodies.size(); j++)
            {
                if (!bodies[j].alive)
                    continue;

                float dx = bodies[j].x - bodies[i].x;
                float dy = bodies[j].y - bodies[i].y;
                float r2 = dx * dx + dy * dy + eps2;
                float r = std::sqrt(r2);
                float invR = 1.0f / r;
                float invR3 = invR * invR * invR;

                float sx = G * dx * invR3;
                float sy = G * dy * invR3;
                ax[i] += sx * bodies[j].mass;
                ay[i] += sy * bodies[j].mass;
                ax[j] -= sx * bodies[i].mass;
                ay[j] -= sy * bodies[i].mass;

                // very gentle capture nudge only for BH encounters
                bool capturePair = (bodies[i].kind == BodyKind::BlackHole || bodies[j].kind == BodyKind::BlackHole);
                if (capturePair)
                {
                    const float captureR = (bodies[i].radius + bodies[j].radius) * 2.4f;
                    if (r < captureR)
                    {
                        float tx = -dy * invR;
                        float ty = dx * invR;
                        float circV = std::sqrt(std::max(0.0f, G * (bodies[i].mass + bodies[j].mass) / std::max(8.0f, r)));
                        float k = 0.02f;
                        bodies[i].vx += tx * circV * k;
                        bodies[i].vy += ty * circV * k;
                        bodies[j].vx -= tx * circV * k;
                        bodies[j].vy -= ty * circV * k;
                    }
                }
            }
        }
    };

    std::vector<float> ax, ay;
    for (int step = 0; step < substeps; step++)
    {
        computeAcc(ax, ay);
        for (size_t i = 0; i < bodies.size(); i++)
        {
            if (!bodies[i].alive)
                continue;
            bodies[i].vx += 0.5f * h * ax[i];
            bodies[i].vy += 0.5f * h * ay[i];
        }

        for (size_t i = 0; i < bodies.size(); i++)
        {
            if (!bodies[i].alive)
                continue;
            bodies[i].x += bodies[i].vx * h;
            bodies[i].y += bodies[i].vy * h;
        }

        computeAcc(ax, ay);
        for (size_t i = 0; i < bodies.size(); i++)
        {
            if (!bodies[i].alive)
                continue;
            bodies[i].vx += 0.5f * h * ax[i];
            bodies[i].vy += 0.5f * h * ay[i];
        }
    }

    // Hill-sphere stabilization for moons around parent planets.
    if (!bodies.empty())
    {
        const SimBody &star = bodies[0];
        for (size_t i = 1; i < bodies.size(); i++)
        {
            if (!bodies[i].alive || bodies[i].kind != BodyKind::Moon)
                continue;
            if (bodies[i].parent <= 0 || bodies[i].parent >= static_cast<int>(bodies.size()))
                continue;

            SimBody &moon = bodies[i];
            SimBody &planet = bodies[moon.parent];

            float psx = planet.x - star.x;
            float psy = planet.y - star.y;
            float rps = std::sqrt(psx * psx + psy * psy);

            float mpx = moon.x - planet.x;
            float mpy = moon.y - planet.y;
            float rmp = std::sqrt(mpx * mpx + mpy * mpy);

            float hill = rps * std::cbrt(std::max(1.0e-6f, planet.mass / (3.0f * std::max(1.0f, star.mass))));
            float safeR = 0.85f * hill;
            float detachR = 1.35f * hill;

            if (rmp > safeR && rmp < detachR)
            {
                // re-circularize around planet when drifting near Hill boundary
                float tx = -mpy / std::max(1.0f, rmp);
                float ty = mpx / std::max(1.0f, rmp);
                float vc = std::sqrt((G * planet.mass) / std::max(6.0f, rmp));
                moon.vx = moon.vx * 0.85f + (planet.vx + tx * vc) * 0.15f;
                moon.vy = moon.vy * 0.85f + (planet.vy + ty * vc) * 0.15f;
            }
            else if (rmp >= detachR)
            {
                // escaped moon becomes minor planet
                moon.kind = BodyKind::Planet;
                moon.parent = -1;
                moon.mass *= 1.2f;
                moon.radius = std::max(6.0f, moon.radius);
                moon.color = {170, 190, 220, 255};
            }
        }
    }
}

void SolarSystemLevel::increasePopulation()
{
    spawnAt((static_cast<float>(rand()) / RAND_MAX - 0.5f) * 900.0f,
            (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 900.0f,
            false);
}

void SolarSystemLevel::decreasePopulation()
{
    for (auto it = bodies.end(); it != bodies.begin();)
    {
        --it;
        if (it->kind == BodyKind::Moon)
        {
            bodies.erase(it);
            break;
        }
    }

    // Rebuild parent links after erase.
    for (auto &b : bodies)
        if (b.kind == BodyKind::Moon && b.parent >= static_cast<int>(bodies.size()))
            b.parent = -1;
}

void SolarSystemLevel::increaseTimeScale()
{
    timeScale = std::min(8.0f, timeScale + 0.1f);
}

void SolarSystemLevel::decreaseTimeScale()
{
    timeScale = std::max(0.2f, timeScale - 0.1f);
}

void SolarSystemLevel::draw(SDL_Renderer *renderer, const Camera &camera) const
{
    SDL_SetRenderDrawColor(renderer, 7, 8, 15, 255);
    SDL_RenderClear(renderer);

    for (size_t i = 1; i < bodies.size(); i++)
    {
        if (!bodies[i].alive)
            continue;
        SDL_FPoint p = camera.worldToScreen(bodies[i].x, bodies[i].y);
        const int rr = std::max(2, static_cast<int>(bodies[i].radius * 0.35f));
        SDL_SetRenderDrawColor(renderer, bodies[i].color.r, bodies[i].color.g, bodies[i].color.b, 255);
        drawFilledCircle(renderer, static_cast<int>(p.x), static_cast<int>(p.y), rr);
    }

    SDL_FPoint s = camera.worldToScreen(bodies[0].x, bodies[0].y);
    SDL_SetRenderDrawColor(renderer, 255, 220, 120, 255);
    drawFilledCircle(renderer, static_cast<int>(s.x), static_cast<int>(s.y), 8);

    SDL_RenderPresent(renderer);
}

bool SolarSystemLevel::findBodyAtScreen(const Camera &camera, int sx, int sy, SimBody &outBody) const
{
    float best = 1.0e9f;
    bool found = false;
    for (const auto &b : bodies)
    {
        if (!b.alive)
            continue;
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

std::unique_ptr<Level> SolarSystemLevel::createChildFromBody(const SimBody &body) const
{
    if (body.kind == BodyKind::Planet)
        return std::make_unique<PlanetLevel>(body.mass * 8.0f);
    if (body.kind == BodyKind::BlackHole)
        return std::make_unique<BlackHoleLevel>();
    return nullptr;
}

void SolarSystemLevel::applyMouseGravity(float wx, float wy, float sign)
{
    for (auto &b : bodies)
    {
        if (!b.alive || b.kind == BodyKind::Star)
            continue;
        float dx = wx - b.x;
        float dy = wy - b.y;
        float d2 = dx * dx + dy * dy;
        if (d2 < 2.0f)
            continue;
        float d = std::sqrt(d2);
        float f = sign * std::max(0.0f, 1.0f - d / 600.0f) * 0.45f;
        b.vx += (dx / d) * f;
        b.vy += (dy / d) * f;
    }
}

void SolarSystemLevel::spawnAt(float wx, float wy, bool blackHole)
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
        b.mass = 25000.0f;
        b.radius = 22.0f;
        b.color = {20, 20, 26, 255};
    }
    else
    {
        b.kind = BodyKind::Planet;
        b.mass = 120.0f;
        b.radius = 11.0f;
        b.color = {160, 190, 255, 255};
        const float r = std::sqrt(b.x * b.x + b.y * b.y);
        const float v = std::sqrt((G * bodies[0].mass) / std::max(30.0f, r + softening));
        if (r > 0.01f)
        {
            b.vx = -b.y / r * v;
            b.vy = b.x / r * v;
        }
    }
    bodies.push_back(b);
}
