#include "UniverseSim.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace
{
    constexpr float PI = 3.1415926535f;

    void drawFilledCircle(SDL_Renderer *renderer, int cx, int cy, int radius)
    {
        for (int y = -radius; y <= radius; y++)
        {
            int xx = static_cast<int>(std::sqrt(radius * radius - y * y));
            SDL_RenderDrawLine(renderer, cx - xx, cy + y, cx + xx, cy + y);
        }
    }
}

UniverseSim::UniverseSim(int width, int height)
    : width(width), height(height),
      G(18.0f), softening(18.0f), timeScale(1.0f), bodyCap(1400), trailsEnabled(true),
      mouseX(width / 2), mouseY(height / 2), mouseMode(0)
{
    randomize();
}

void UniverseSim::spawnGalaxy()
{
    bodies.clear();
    const float cx = width * 0.5f;
    const float cy = height * 0.5f;

    Body coreBH;
    coreBH.x = cx;
    coreBH.y = cy;
    coreBH.vx = 0.0f;
    coreBH.vy = 0.0f;
    coreBH.mass = 15000.0f;
    coreBH.radius = 7.5f;
    coreBH.color = {18, 18, 24, 255};
    coreBH.kind = 2;
    coreBH.alive = true;
    bodies.push_back(coreBH);

    const int count = 900;
    const float diskScaleLength = 165.0f;
    const float diskMassTotal = 6500.0f;
    const float bulgeMassTotal = 1800.0f;
    const float bulgeScale = 85.0f;

    for (int i = 0; i < count; i++)
    {
        const float u = std::max(1.0e-5f, static_cast<float>(rand()) / RAND_MAX);
        float r = -diskScaleLength * std::log(u); // exponential disk
        r = std::clamp(r, 28.0f, 500.0f);
        const float a = (static_cast<float>(rand()) / RAND_MAX) * 2.0f * PI;

        Body b;
        b.x = cx + std::cos(a) * r;
        b.y = cy + std::sin(a) * r;

        // Rotation curve from enclosed mass model:
        // M(<r) = M_bh + M_disk(<r) + M_bulge(<r)
        const float x = r / diskScaleLength;
        const float enclosedDisk = diskMassTotal * (1.0f - std::exp(-x) * (1.0f + x));
        const float enclosedBulge = bulgeMassTotal * ((r * r) / ((r + bulgeScale) * (r + bulgeScale)));
        const float enclosed = coreBH.mass + enclosedDisk + enclosedBulge;
        const float v = std::sqrt((G * enclosed) / (r + softening));

        // Small velocity dispersions to keep an organized rotating disk.
        const float vtune = 0.97f + (static_cast<float>(rand()) / RAND_MAX) * 0.08f;
        const float sigmaR = 0.05f + 0.16f * std::exp(-r / 210.0f);
        const float sigmaT = 0.03f + 0.09f * std::exp(-r / 260.0f);
        const float radialKick = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 2.0f * sigmaR;
        const float tangentialKick = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 2.0f * sigmaT;

        const float tx = -std::sin(a);
        const float ty = std::cos(a);
        const float rx = std::cos(a);
        const float ry = std::sin(a);
        b.vx = tx * (v * vtune + tangentialKick) + rx * radialKick;
        b.vy = ty * (v * vtune + tangentialKick) + ry * radialKick;

        if (rand() % 100 < 5)
        {
            b.kind = 1;
            b.mass = 12.0f + (static_cast<float>(rand()) / RAND_MAX) * 40.0f;
            b.radius = 1.5f + (static_cast<float>(rand()) / RAND_MAX) * 1.5f;
            b.color = {255, static_cast<Uint8>(180 + rand() % 70), static_cast<Uint8>(90 + rand() % 80), 255};
        }
        else
        {
            b.kind = 0;
            b.mass = 0.9f + (static_cast<float>(rand()) / RAND_MAX) * 2.4f;
            b.radius = 0.8f;
            b.color = {static_cast<Uint8>(110 + rand() % 80), static_cast<Uint8>(140 + rand() % 80), 255, 255};
        }

        b.alive = true;
        bodies.push_back(b);
    }
}

void UniverseSim::randomize()
{
    spawnGalaxy();
}

void UniverseSim::clear()
{
    bodies.clear();
}

void UniverseSim::spawnBlackHoleAt(int px, int py)
{
    if (px < 0 || py < 0 || px >= width || py >= height)
        return;
    if (static_cast<int>(bodies.size()) >= bodyCap)
        return;

    Body b;
    b.x = static_cast<float>(px);
    b.y = static_cast<float>(py);
    b.vx = 0.0f;
    b.vy = 0.0f;
    b.mass = 12000.0f;
    b.radius = 9.0f;
    b.color = {15, 15, 20, 255};
    b.kind = 2;
    b.alive = true;
    bodies.push_back(b);
}

void UniverseSim::spawnStarAt(int px, int py)
{
    if (px < 0 || py < 0 || px >= width || py >= height)
        return;
    if (static_cast<int>(bodies.size()) >= bodyCap)
        return;

    Body b;
    b.x = static_cast<float>(px);
    b.y = static_cast<float>(py);
    const float a = (static_cast<float>(rand()) / RAND_MAX) * 2.0f * PI;
    const float s = (static_cast<float>(rand()) / RAND_MAX) * 1.5f;
    b.vx = std::cos(a) * s;
    b.vy = std::sin(a) * s;
    b.mass = 60.0f + (static_cast<float>(rand()) / RAND_MAX) * 90.0f;
    b.radius = 2.5f + (static_cast<float>(rand()) / RAND_MAX) * 2.2f;
    b.color = {255, static_cast<Uint8>(180 + rand() % 70), static_cast<Uint8>(90 + rand() % 80), 255};
    b.kind = 1;
    b.alive = true;
    bodies.push_back(b);
}

void UniverseSim::removeNearestBody(int px, int py)
{
    if (bodies.empty())
        return;

    int best = -1;
    float bestD2 = 1.0e12f;
    for (int i = 0; i < static_cast<int>(bodies.size()); i++)
    {
        float dx = bodies[i].x - px;
        float dy = bodies[i].y - py;
        float d2 = dx * dx + dy * dy;
        if (d2 < bestD2)
        {
            bestD2 = d2;
            best = i;
        }
    }
    if (best >= 0)
        bodies.erase(bodies.begin() + best);
}

void UniverseSim::checkMergers()
{
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
            float d2 = dx * dx + dy * dy;
            float rr = bodies[i].radius + bodies[j].radius;
            if (d2 > rr * rr)
                continue;

            const float rvx = bodies[j].vx - bodies[i].vx;
            const float rvy = bodies[j].vy - bodies[i].vy;
            const float relV = std::sqrt(rvx * rvx + rvy * rvy);
            const float vEsc = std::sqrt(std::max(0.0f, 2.0f * G * (bodies[i].mass + bodies[j].mass) / (rr + 0.35f * softening)));

            if (bodies[i].kind == 2 || bodies[j].kind == 2)
            {
                size_t bh = (bodies[i].kind == 2) ? i : j;
                size_t other = (bh == i) ? j : i;
                bodies[bh].mass += bodies[other].mass * 0.8f;
                bodies[bh].radius = std::min(34.0f, std::sqrt(bodies[bh].radius * bodies[bh].radius + bodies[other].radius * bodies[other].radius));
                bodies[other].alive = false;
                continue;
            }

            // Collisionless stellar dynamics: no star-star/particle-star merging.
            continue;

        }
    }
}

void UniverseSim::update(float dt)
{
    if (bodies.empty())
        return;

    const float dtFrame = std::clamp(dt * 60.0f * timeScale, 0.03f, 1.8f);
    const int substeps = (dtFrame > 1.0f) ? 3 : ((dtFrame > 0.5f) ? 2 : 1);
    const float h = dtFrame / static_cast<float>(substeps);

    auto computeAcceleration = [&](std::vector<float> &ax, std::vector<float> &ay)
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
                float invR = 1.0f / std::sqrt(r2);
                float invR3 = invR * invR * invR;

                float sx = G * dx * invR3;
                float sy = G * dy * invR3;

                ax[i] += sx * bodies[j].mass;
                ay[i] += sy * bodies[j].mass;
                ax[j] -= sx * bodies[i].mass;
                ay[j] -= sy * bodies[i].mass;
            }

            if (mouseMode != 0)
            {
                float dx = static_cast<float>(mouseX) - bodies[i].x;
                float dy = static_cast<float>(mouseY) - bodies[i].y;
                float d2 = dx * dx + dy * dy;
                if (d2 > 4.0f)
                {
                    float d = std::sqrt(d2);
                    float sign = static_cast<float>(mouseMode);
                    float f = sign * std::max(0.0f, 1.0f - d / 360.0f) * 95.0f;
                    ax[i] += (dx / d) * f / std::max(1.0f, bodies[i].mass);
                    ay[i] += (dy / d) * f / std::max(1.0f, bodies[i].mass);
                }
            }
        }
    };

    std::vector<float> ax, ay;
    for (int step = 0; step < substeps; step++)
    {
        computeAcceleration(ax, ay);

        // Kick (half)
        for (size_t i = 0; i < bodies.size(); i++)
        {
            if (!bodies[i].alive)
                continue;
            if (bodies[i].kind == 2)
                continue;

            bodies[i].vx += 0.5f * h * ax[i];
            bodies[i].vy += 0.5f * h * ay[i];
        }

        // Drift
        for (size_t i = 0; i < bodies.size(); i++)
        {
            if (!bodies[i].alive)
                continue;

            bodies[i].x += h * bodies[i].vx;
            bodies[i].y += h * bodies[i].vy;

            if (bodies[i].x < 0)
                bodies[i].x += width;
            if (bodies[i].x >= width)
                bodies[i].x -= width;
            if (bodies[i].y < 0)
                bodies[i].y += height;
            if (bodies[i].y >= height)
                bodies[i].y -= height;
        }

        computeAcceleration(ax, ay);

        // Kick (half)
        for (size_t i = 0; i < bodies.size(); i++)
        {
            if (!bodies[i].alive)
                continue;
            if (bodies[i].kind == 2)
                continue;

            bodies[i].vx += 0.5f * h * ax[i];
            bodies[i].vy += 0.5f * h * ay[i];

            const float vmax = 120.0f;
            float v = std::sqrt(bodies[i].vx * bodies[i].vx + bodies[i].vy * bodies[i].vy);
            if (v > vmax)
            {
                bodies[i].vx = (bodies[i].vx / v) * vmax;
                bodies[i].vy = (bodies[i].vy / v) * vmax;
            }
        }
    }

    checkMergers();

    bodies.erase(
        std::remove_if(bodies.begin(), bodies.end(), [](const Body &b)
                       { return !b.alive; }),
        bodies.end());

    if (static_cast<int>(bodies.size()) > bodyCap)
        bodies.resize(bodyCap);
}

void UniverseSim::draw(SDL_Renderer *renderer)
{
    if (trailsEnabled)
    {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 5, 7, 12, 22);
        SDL_RenderFillRect(renderer, nullptr);
    }
    else
    {
        SDL_SetRenderDrawColor(renderer, 5, 7, 12, 255);
        SDL_RenderClear(renderer);
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    for (const auto &b : bodies)
    {
        if (b.kind == 1)
        {
            SDL_SetRenderDrawColor(renderer, b.color.r, b.color.g, b.color.b, 30);
            drawFilledCircle(renderer, static_cast<int>(b.x), static_cast<int>(b.y), static_cast<int>(b.radius) + 7);
        }
        if (b.kind == 2)
        {
            SDL_SetRenderDrawColor(renderer, 80, 90, 140, 35);
            drawFilledCircle(renderer, static_cast<int>(b.x), static_cast<int>(b.y), static_cast<int>(b.radius) + 16);
            SDL_SetRenderDrawColor(renderer, 220, 160, 90, 28);
            drawFilledCircle(renderer, static_cast<int>(b.x), static_cast<int>(b.y), static_cast<int>(b.radius) + 26);
        }

        SDL_SetRenderDrawColor(renderer, b.color.r, b.color.g, b.color.b, 255);
        drawFilledCircle(renderer, static_cast<int>(b.x), static_cast<int>(b.y), static_cast<int>(std::max(1.0f, b.radius)));
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    SDL_RenderPresent(renderer);
}

void UniverseSim::increaseSpeed()
{
    timeScale = std::min(3.5f, timeScale + 0.1f);
}

void UniverseSim::decreaseSpeed()
{
    timeScale = std::max(0.2f, timeScale - 0.1f);
}

void UniverseSim::toggleTrails()
{
    trailsEnabled = !trailsEnabled;
}

void UniverseSim::setMouseInteraction(int x, int y, int mode)
{
    mouseX = x;
    mouseY = y;
    mouseMode = mode;
}

void UniverseSim::clearMouseInteraction()
{
    mouseMode = 0;
}

int UniverseSim::getBlackHoleCount() const
{
    int count = 0;
    for (const auto &b : bodies)
        if (b.kind == 2)
            count++;
    return count;
}

int UniverseSim::getStarCount() const
{
    int count = 0;
    for (const auto &b : bodies)
        if (b.kind == 1)
            count++;
    return count;
}
