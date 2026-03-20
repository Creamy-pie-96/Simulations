#include "UniverseSim.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace
{
    constexpr float PI = 3.1415926535f;
    constexpr float PROJ_R0 = 420.0f; // world distance where compression starts
    constexpr float PROJ_S0 = 300.0f; // pixel scale for logarithmic projection
    constexpr float DISK_SCALE_LENGTH = 4200.0f;
    constexpr float DISK_MASS_TOTAL = 7.5e6f;
    constexpr float BULGE_MASS_TOTAL = 2.4e6f;
    constexpr float BULGE_SCALE = 1100.0f;

    float projectRadius(float worldR)
    {
        return std::log1p(std::max(0.0f, worldR) / PROJ_R0) * PROJ_S0;
    }

    float unprojectRadius(float screenR)
    {
        return PROJ_R0 * (std::exp(std::max(0.0f, screenR) / PROJ_S0) - 1.0f);
    }

    SDL_FPoint worldToScreen(float wx, float wy, float cx, float cy)
    {
        const float r = std::sqrt(wx * wx + wy * wy);
        if (r < 1.0e-5f)
            return {cx, cy};

        const float rs = projectRadius(r);
        const float ux = wx / r;
        const float uy = wy / r;
        return {cx + ux * rs, cy + uy * rs};
    }

    SDL_FPoint screenToWorld(float sx, float sy, float cx, float cy)
    {
        const float dx = sx - cx;
        const float dy = sy - cy;
        const float rs = std::sqrt(dx * dx + dy * dy);
        if (rs < 1.0e-5f)
            return {0.0f, 0.0f};

        const float r = unprojectRadius(rs);
        const float ux = dx / rs;
        const float uy = dy / rs;
        return {ux * r, uy * r};
    }

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
      G(0.012f), softening(160.0f), timeScale(1.0f), bodyCap(1600), trailsEnabled(true),
      mouseX(width / 2), mouseY(height / 2), mouseMode(0)
{
    randomize();
}

void UniverseSim::spawnGalaxy()
{
    bodies.clear();
    const float cx = 0.0f;
    const float cy = 0.0f;

    Body coreBH;
    coreBH.x = cx;
    coreBH.y = cy;
    coreBH.vx = 0.0f;
    coreBH.vy = 0.0f;
    coreBH.mass = 4.2e6f;
    coreBH.radius = 65.0f;
    coreBH.color = {18, 18, 24, 255};
    coreBH.kind = 2;
    coreBH.alive = true;
    bodies.push_back(coreBH);

    const int count = 1200;
    for (int i = 0; i < count; i++)
    {
        const float u = std::max(1.0e-5f, static_cast<float>(rand()) / RAND_MAX);
        float r = -DISK_SCALE_LENGTH * std::log(u); // exponential disk
        r = std::clamp(r, 260.0f, 26000.0f);
        const float a = (static_cast<float>(rand()) / RAND_MAX) * 2.0f * PI;

        Body b;
        b.x = cx + std::cos(a) * r;
        b.y = cy + std::sin(a) * r;

        // Rotation curve from enclosed mass model:
        // M(<r) = M_bh + M_disk(<r) + M_bulge(<r)
        const float x = r / DISK_SCALE_LENGTH;
        const float enclosedDisk = DISK_MASS_TOTAL * (1.0f - std::exp(-x) * (1.0f + x));
        const float enclosedBulge = BULGE_MASS_TOTAL * ((r * r) / ((r + BULGE_SCALE) * (r + BULGE_SCALE)));
        const float enclosed = coreBH.mass + enclosedDisk + enclosedBulge;
        const float v = std::sqrt((G * enclosed) / (r + softening));

        // Small velocity dispersions to keep an organized rotating disk.
        const float vtune = 0.98f + (static_cast<float>(rand()) / RAND_MAX) * 0.06f;
        const float sigmaR = 0.06f + 0.25f * std::exp(-r / 5200.0f);
        const float sigmaT = 0.04f + 0.16f * std::exp(-r / 5800.0f);
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
            b.mass = 20.0f + (static_cast<float>(rand()) / RAND_MAX) * 120.0f;
            b.radius = 22.0f + (static_cast<float>(rand()) / RAND_MAX) * 30.0f;
            b.color = {255, static_cast<Uint8>(180 + rand() % 70), static_cast<Uint8>(90 + rand() % 80), 255};
        }
        else
        {
            b.kind = 0;
            b.mass = 2.0f + (static_cast<float>(rand()) / RAND_MAX) * 8.0f;
            b.radius = 10.0f;
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

    const float cx = width * 0.5f;
    const float cy = height * 0.5f;
    const SDL_FPoint w = screenToWorld(static_cast<float>(px), static_cast<float>(py), cx, cy);

    Body b;
    b.x = w.x;
    b.y = w.y;
    b.vx = 0.0f;
    b.vy = 0.0f;
    b.mass = 3.0e6f;
    b.radius = 55.0f;
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

    const float cx = width * 0.5f;
    const float cy = height * 0.5f;
    const SDL_FPoint w = screenToWorld(static_cast<float>(px), static_cast<float>(py), cx, cy);

    Body b;
    b.x = w.x;
    b.y = w.y;
    const float a = (static_cast<float>(rand()) / RAND_MAX) * 2.0f * PI;
    const float s = (static_cast<float>(rand()) / RAND_MAX) * 0.8f;
    b.vx = std::cos(a) * s;
    b.vy = std::sin(a) * s;
    b.mass = 20.0f + (static_cast<float>(rand()) / RAND_MAX) * 100.0f;
    b.radius = 22.0f + (static_cast<float>(rand()) / RAND_MAX) * 28.0f;
    b.color = {255, static_cast<Uint8>(180 + rand() % 70), static_cast<Uint8>(90 + rand() % 80), 255};
    b.kind = 1;
    b.alive = true;
    bodies.push_back(b);
}

void UniverseSim::removeNearestBody(int px, int py)
{
    if (bodies.empty())
        return;

    const float cx = width * 0.5f;
    const float cy = height * 0.5f;
    const SDL_FPoint w = screenToWorld(static_cast<float>(px), static_cast<float>(py), cx, cy);

    int best = -1;
    float bestD2 = 1.0e12f;
    for (int i = 0; i < static_cast<int>(bodies.size()); i++)
    {
        float dx = bodies[i].x - w.x;
        float dy = bodies[i].y - w.y;
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

            if (bodies[i].kind == 2 || bodies[j].kind == 2)
            {
                size_t bh = (bodies[i].kind == 2) ? i : j;
                size_t other = (bh == i) ? j : i;
                const float m1 = bodies[bh].mass;
                const float m2 = bodies[other].mass * 0.8f;
                const float m = m1 + m2;
                bodies[bh].vx = (bodies[bh].vx * m1 + bodies[other].vx * m2) / m;
                bodies[bh].vy = (bodies[bh].vy * m1 + bodies[other].vy * m2) / m;
                bodies[bh].x = (bodies[bh].x * m1 + bodies[other].x * m2) / m;
                bodies[bh].y = (bodies[bh].y * m1 + bodies[other].y * m2) / m;
                bodies[bh].mass = m;
                bodies[bh].radius = std::min(220.0f, std::sqrt(bodies[bh].radius * bodies[bh].radius + bodies[other].radius * bodies[other].radius));
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
    const float cx = width * 0.5f;
    const float cy = height * 0.5f;
    const SDL_FPoint mouseWorld = screenToWorld(static_cast<float>(mouseX), static_cast<float>(mouseY), cx, cy);

    auto computeAcceleration = [&](std::vector<float> &ax, std::vector<float> &ay)
    {
        ax.assign(bodies.size(), 0.0f);
        ay.assign(bodies.size(), 0.0f);

        const float eps2 = softening * softening;

        // Dominant center for smooth potential (first black hole).
        int centerBH = -1;
        for (int i = 0; i < static_cast<int>(bodies.size()); i++)
        {
            if (bodies[i].alive && bodies[i].kind == 2)
            {
                centerBH = i;
                break;
            }
        }

        for (size_t i = 0; i < bodies.size(); i++)
        {
            if (!bodies[i].alive)
                continue;

            // Smooth collisionless galactic potential around the central black hole.
            if (centerBH >= 0 && static_cast<int>(i) != centerBH)
            {
                const Body &bh = bodies[centerBH];
                float dx = bh.x - bodies[i].x;
                float dy = bh.y - bodies[i].y;
                float r2 = dx * dx + dy * dy + eps2;
                float r = std::sqrt(r2);
                float invR = 1.0f / r;

                const float x = r / DISK_SCALE_LENGTH;
                const float enclosedDisk = DISK_MASS_TOTAL * (1.0f - std::exp(-x) * (1.0f + x));
                const float enclosedBulge = BULGE_MASS_TOTAL * ((r * r) / ((r + BULGE_SCALE) * (r + BULGE_SCALE)));
                const float enclosed = bh.mass + enclosedDisk + enclosedBulge;
                const float amag = G * enclosed / r2;

                ax[i] += dx * invR * amag;
                ay[i] += dy * invR * amag;
            }

            // Additional direct black-hole interactions (for multiple BHs).
            for (size_t j = i + 1; j < bodies.size(); j++)
            {
                if (!bodies[j].alive)
                    continue;
                if (!(bodies[i].kind == 2 && bodies[j].kind == 2))
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
                float dx = mouseWorld.x - bodies[i].x;
                float dy = mouseWorld.y - bodies[i].y;
                float d2 = dx * dx + dy * dy;
                if (d2 > 4.0f)
                {
                    float d = std::sqrt(d2);
                    float sign = static_cast<float>(mouseMode);
                    float f = sign * std::max(0.0f, 1.0f - d / 9000.0f) * 80.0f;
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

            const float r = std::sqrt(bodies[i].x * bodies[i].x + bodies[i].y * bodies[i].y);
            if (r > 42000.0f)
                bodies[i].alive = false;
        }

        computeAcceleration(ax, ay);

        // Kick (half)
        for (size_t i = 0; i < bodies.size(); i++)
        {
            if (!bodies[i].alive)
                continue;

            bodies[i].vx += 0.5f * h * ax[i];
            bodies[i].vy += 0.5f * h * ay[i];

            const float vmax = 7.0f;
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
    const float cx = width * 0.5f;
    const float cy = height * 0.5f;

    for (const auto &b : bodies)
    {
        SDL_FPoint s = worldToScreen(b.x, b.y, cx, cy);
        const float baseR = std::max(1.0f, 0.85f + 0.15f * std::sqrt(std::max(0.0f, b.radius)));
        const int rpx = std::max(1, static_cast<int>(baseR));

        if (b.kind == 1)
        {
            SDL_SetRenderDrawColor(renderer, b.color.r, b.color.g, b.color.b, 30);
            drawFilledCircle(renderer, static_cast<int>(s.x), static_cast<int>(s.y), rpx + 5);
        }
        if (b.kind == 2)
        {
            SDL_SetRenderDrawColor(renderer, 80, 90, 140, 35);
            drawFilledCircle(renderer, static_cast<int>(s.x), static_cast<int>(s.y), rpx + 11);
            SDL_SetRenderDrawColor(renderer, 220, 160, 90, 28);
            drawFilledCircle(renderer, static_cast<int>(s.x), static_cast<int>(s.y), rpx + 18);
        }

        SDL_SetRenderDrawColor(renderer, b.color.r, b.color.g, b.color.b, 255);
        drawFilledCircle(renderer, static_cast<int>(s.x), static_cast<int>(s.y), rpx);
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
