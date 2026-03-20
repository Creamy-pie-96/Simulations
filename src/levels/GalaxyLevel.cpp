#include "levels/GalaxyLevel.hpp"

#include "levels/BlackHoleLevel.hpp"
#include "levels/SolarSystemLevel.hpp"

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

GalaxyLevel::GalaxyLevel(float seedMass)
    : G(0.012f), softening(180.0f),
      diskMass(std::max(4.5e6f, seedMass * 0.06f)),
      bulgeMass(std::max(1.8e6f, seedMass * 0.025f)),
      diskScale(4200.0f), bulgeScale(1000.0f), armCount(4), twistFactor(0.0016f), timeScale(1.0f)
{
    SimBody bh;
    bh.kind = BodyKind::BlackHole;
    bh.x = 0.0f;
    bh.y = 0.0f;
    bh.mass = 4.0e6f + std::max(0.0f, seedMass * 0.003f);
    bh.radius = 65.0f;
    bh.color = {18, 18, 24, 255};
    bodies.push_back(bh);

    const int count = 1500;
    for (int i = 0; i < count; i++)
    {
        const float u = std::max(1.0e-5f, static_cast<float>(rand()) / RAND_MAX);
        const float r = std::clamp(-diskScale * std::log(u), 260.0f, 25000.0f);
        const int arm = rand() % armCount;
        const float armBase = (6.2831853f / static_cast<float>(armCount)) * arm;
        const float jitter = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.42f;
        const float a = armBase + r * twistFactor + jitter;

        SimBody s;
        s.kind = BodyKind::Star;
        s.x = std::cos(a) * r;
        s.y = std::sin(a) * r;
        s.mass = 4.0f + (static_cast<float>(rand()) / RAND_MAX) * 45.0f;
        s.radius = 20.0f + (static_cast<float>(rand()) / RAND_MAX) * 28.0f;
        s.color = {255, static_cast<Uint8>(235 + rand() % 20), static_cast<Uint8>(235 + rand() % 20), 255};

        const float x = r / diskScale;
        const float enclosedDisk = diskMass * (1.0f - std::exp(-x) * (1.0f + x));
        const float enclosedBulge = bulgeMass * ((r * r) / ((r + bulgeScale) * (r + bulgeScale)));
        const float enclosed = bh.mass + enclosedDisk + enclosedBulge;
        const float v = std::sqrt((G * enclosed) / (r + softening));

        const float tangentialNoise = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.18f;
        const float radialNoise = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.25f;
        const float tx = -std::sin(a);
        const float ty = std::cos(a);
        const float rx = std::cos(a);
        const float ry = std::sin(a);
        s.vx = tx * (v + tangentialNoise) + rx * radialNoise;
        s.vy = ty * (v + tangentialNoise) + ry * radialNoise;

        bodies.push_back(s);
    }
}

void GalaxyLevel::update(float dt)
{
    const float dtFrame = std::clamp(dt * 60.0f * timeScale, 0.03f, 1.9f);
    const int substeps = (dtFrame > 1.0f) ? 3 : ((dtFrame > 0.5f) ? 2 : 1);
    const float h = dtFrame / static_cast<float>(substeps);

    auto acc = [&](std::vector<float> &ax, std::vector<float> &ay)
    {
        ax.assign(bodies.size(), 0.0f);
        ay.assign(bodies.size(), 0.0f);
        const float eps2 = softening * softening;

        for (size_t i = 1; i < bodies.size(); i++)
        {
            if (!bodies[i].alive)
                continue;

            const SimBody &bh = bodies[0];
            float dx = bh.x - bodies[i].x;
            float dy = bh.y - bodies[i].y;
            float r2 = dx * dx + dy * dy + eps2;
            float r = std::sqrt(r2);
            float invR = 1.0f / r;

            float x = r / diskScale;
            float enclosedDisk = diskMass * (1.0f - std::exp(-x) * (1.0f + x));
            float enclosedBulge = bulgeMass * ((r * r) / ((r + bulgeScale) * (r + bulgeScale)));
            float enclosed = bh.mass + enclosedDisk + enclosedBulge;
            float amag = G * enclosed / r2;

            ax[i] += (dx * invR) * amag;
            ay[i] += (dy * invR) * amag;
        }
    };

    std::vector<float> ax, ay;
    for (int step = 0; step < substeps; step++)
    {
        acc(ax, ay);
        for (size_t i = 1; i < bodies.size(); i++)
        {
            if (!bodies[i].alive)
                continue;
            bodies[i].vx += ax[i] * h * 0.5f;
            bodies[i].vy += ay[i] * h * 0.5f;
            bodies[i].x += bodies[i].vx * h;
            bodies[i].y += bodies[i].vy * h;
        }

        acc(ax, ay);
        for (size_t i = 1; i < bodies.size(); i++)
        {
            if (!bodies[i].alive)
                continue;
            bodies[i].vx += ax[i] * h * 0.5f;
            bodies[i].vy += ay[i] * h * 0.5f;
        }
    }

    // Accretion: stars crossing event-horizon radius are consumed.
    if (!bodies.empty())
    {
        SimBody &bh = bodies[0];
        const float captureR = std::max(120.0f, bh.radius * 2.2f);
        const float captureR2 = captureR * captureR;
        for (size_t i = 1; i < bodies.size(); i++)
        {
            if (!bodies[i].alive || bodies[i].kind != BodyKind::Star)
                continue;

            float dx = bodies[i].x - bh.x;
            float dy = bodies[i].y - bh.y;
            float d2 = dx * dx + dy * dy;
            if (d2 <= captureR2)
            {
                const float m1 = bh.mass;
                const float m2 = bodies[i].mass;
                const float m = m1 + m2;
                bh.vx = (bh.vx * m1 + bodies[i].vx * m2) / m;
                bh.vy = (bh.vy * m1 + bodies[i].vy * m2) / m;
                bh.mass = m;
                bh.radius = std::min(140.0f, std::sqrt(bh.radius * bh.radius + bodies[i].radius * bodies[i].radius * 0.25f));
                bodies[i].alive = false;
            }
        }

        bodies.erase(std::remove_if(bodies.begin() + 1, bodies.end(), [](const SimBody &b)
                                    { return !b.alive; }),
                     bodies.end());
    }
}

void GalaxyLevel::increasePopulation()
{
    const int addN = 80;
    for (int i = 0; i < addN; i++)
    {
        const float u = std::max(1.0e-5f, static_cast<float>(rand()) / RAND_MAX);
        const float r = std::clamp(-diskScale * std::log(u), 260.0f, 25000.0f);
        const int arm = rand() % armCount;
        const float armBase = (6.2831853f / static_cast<float>(armCount)) * arm;
        const float jitter = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.42f;
        const float a = armBase + r * twistFactor + jitter;

        SimBody s;
        s.kind = BodyKind::Star;
        s.x = std::cos(a) * r;
        s.y = std::sin(a) * r;
        s.mass = 4.0f + (static_cast<float>(rand()) / RAND_MAX) * 45.0f;
        s.radius = 20.0f + (static_cast<float>(rand()) / RAND_MAX) * 28.0f;
        s.color = {255, static_cast<Uint8>(235 + rand() % 20), static_cast<Uint8>(235 + rand() % 20), 255};

        const float x = r / diskScale;
        const float enclosedDisk = diskMass * (1.0f - std::exp(-x) * (1.0f + x));
        const float enclosedBulge = bulgeMass * ((r * r) / ((r + bulgeScale) * (r + bulgeScale)));
        const float enclosed = bodies[0].mass + enclosedDisk + enclosedBulge;
        const float v = std::sqrt((G * enclosed) / (r + softening));

        const float tangentialNoise = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.18f;
        const float radialNoise = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.25f;
        const float tx = -std::sin(a);
        const float ty = std::cos(a);
        const float rx = std::cos(a);
        const float ry = std::sin(a);
        s.vx = tx * (v + tangentialNoise) + rx * radialNoise;
        s.vy = ty * (v + tangentialNoise) + ry * radialNoise;

        bodies.push_back(s);
    }
}

void GalaxyLevel::decreasePopulation()
{
    int removed = 0;
    for (auto it = bodies.end(); it != bodies.begin() && removed < 80;)
    {
        --it;
        if (it->kind == BodyKind::Star)
        {
            it = bodies.erase(it);
            removed++;
        }
    }
}

void GalaxyLevel::increaseTimeScale()
{
    timeScale = std::min(8.0f, timeScale + 0.1f);
}

void GalaxyLevel::decreaseTimeScale()
{
    timeScale = std::max(0.2f, timeScale - 0.1f);
}

void GalaxyLevel::draw(SDL_Renderer *renderer, const Camera &camera) const
{
    SDL_SetRenderDrawColor(renderer, 5, 7, 13, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);
    // Probability-cloud style star rendering (similar visual principle to atom cloud).
    for (const auto &b : bodies)
    {
        if (!b.alive || b.kind == BodyKind::BlackHole)
            continue;

        SDL_FPoint p = camera.worldToScreen(b.x, b.y);
        const float lum = std::clamp((b.mass - 4.0f) / 42.0f, 0.0f, 1.0f);
        const Uint8 a = static_cast<Uint8>(20 + lum * 50.0f);
        SDL_SetRenderDrawColor(renderer, b.color.r, b.color.g, b.color.b, a);

        for (int k = 0; k < 4; k++)
        {
            float jx = std::sin((b.x + k * 17.0f) * 0.0047f) * (1.2f + lum * 2.2f);
            float jy = std::cos((b.y + k * 13.0f) * 0.0043f) * (1.2f + lum * 2.2f);
            SDL_RenderDrawPoint(renderer, static_cast<int>(p.x + jx), static_cast<int>(p.y + jy));
        }
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    for (const auto &b : bodies)
    {
        if (!b.alive)
            continue;
        SDL_FPoint p = camera.worldToScreen(b.x, b.y);
        const int rr = (b.kind == BodyKind::BlackHole) ? 7 : 2;

        if (b.kind == BodyKind::BlackHole)
        {
            SDL_SetRenderDrawColor(renderer, 80, 90, 140, 35);
            drawFilledCircle(renderer, static_cast<int>(p.x), static_cast<int>(p.y), rr + 12);
            SDL_SetRenderDrawColor(renderer, 220, 160, 90, 30);
            drawFilledCircle(renderer, static_cast<int>(p.x), static_cast<int>(p.y), rr + 20);
        }

        SDL_SetRenderDrawColor(renderer, b.color.r, b.color.g, b.color.b, 230);
        drawFilledCircle(renderer, static_cast<int>(p.x), static_cast<int>(p.y), rr);
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    SDL_RenderPresent(renderer);
}

bool GalaxyLevel::findBodyAtScreen(const Camera &camera, int sx, int sy, SimBody &outBody) const
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
        if (d2 < best && d2 < 18.0f * 18.0f)
        {
            best = d2;
            outBody = b;
            found = true;
        }
    }
    return found;
}

std::unique_ptr<Level> GalaxyLevel::createChildFromBody(const SimBody &body) const
{
    if (body.kind == BodyKind::Star)
        return std::make_unique<SolarSystemLevel>(body.mass * 300.0f);
    if (body.kind == BodyKind::BlackHole)
        return std::make_unique<BlackHoleLevel>();
    return nullptr;
}

void GalaxyLevel::applyMouseGravity(float wx, float wy, float sign)
{
    for (auto &b : bodies)
    {
        if (!b.alive || b.kind == BodyKind::BlackHole)
            continue;
        float dx = wx - b.x;
        float dy = wy - b.y;
        float d2 = dx * dx + dy * dy;
        if (d2 < 10.0f)
            continue;
        float d = std::sqrt(d2);
        float f = sign * std::max(0.0f, 1.0f - d / 12000.0f) * 0.45f;
        b.vx += (dx / d) * f;
        b.vy += (dy / d) * f;
    }
}

void GalaxyLevel::spawnAt(float wx, float wy, bool blackHole)
{
    SimBody b;
    b.x = wx;
    b.y = wy;
    b.vx = 0.0f;
    b.vy = 0.0f;

    if (blackHole)
    {
        b.kind = BodyKind::BlackHole;
        b.mass = 2.5e6f;
        b.radius = 70.0f;
        b.color = {20, 20, 24, 255};
    }
    else
    {
        b.kind = BodyKind::Star;
        b.mass = 35.0f;
        b.radius = 24.0f;
        b.color = {255, 245, 245, 255};
    }
    bodies.push_back(b);
}
