#include "Boids.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>

static SDL_Color hsvToRgbBoids(float h, float s, float v)
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

Boids::Boids(int width, int height, int count)
    : width(width), height(height), flockCount(5), maxFlocks(16),
      mouseX(0), mouseY(0), mouseMode(0),
      separationRadius(25.0f), alignRadius(50.0f), cohesionRadius(50.0f),
      sepWeight(1.5f), alignWeight(1.0f), cohWeight(1.0f),
      maxSpeed(3.0f), maxForce(0.05f)
{
    boids.resize(std::max(0, count));
    rebuildColors();
    randomize();
}

float Boids::wrapDelta(float d, float size) const
{
    if (d > size * 0.5f)
        d -= size;
    if (d < -size * 0.5f)
        d += size;
    return d;
}

void Boids::rebuildColors()
{
    colors.clear();
    for (int i = 0; i < flockCount; i++)
    {
        float h = (360.0f / flockCount) * i;
        colors.push_back(hsvToRgbBoids(h, 0.85f, 1.0f));
    }
}

void Boids::randomize()
{
    for (auto &b : boids)
    {
        b.x = static_cast<float>(rand() % width);
        b.y = static_cast<float>(rand() % height);

        const float a = static_cast<float>(rand()) / RAND_MAX * 6.2831853f;
        const float s = 0.5f + static_cast<float>(rand()) / RAND_MAX * maxSpeed;
        b.vx = std::cos(a) * s;
        b.vy = std::sin(a) * s;
        b.type = rand() % flockCount;
    }
}

void Boids::clear()
{
    for (auto &b : boids)
    {
        b.x = width * 0.5f;
        b.y = height * 0.5f;
        b.vx = 0.0f;
        b.vy = 0.0f;
        b.type = 0;
    }
}

void Boids::update(float dt)
{
    const float dtScale = std::max(0.1f, dt * 60.0f);

    std::vector<float> ax(boids.size(), 0.0f);
    std::vector<float> ay(boids.size(), 0.0f);

    for (size_t i = 0; i < boids.size(); i++)
    {
        const Boid &me = boids[i];

        float sepX = 0.0f, sepY = 0.0f;
        float aliX = 0.0f, aliY = 0.0f;
        float cohX = 0.0f, cohY = 0.0f;

        int sepCount = 0;
        int aliCount = 0;
        int cohCount = 0;

        for (size_t j = 0; j < boids.size(); j++)
        {
            if (i == j)
                continue;

            const Boid &other = boids[j];
            float dx = wrapDelta(other.x - me.x, static_cast<float>(width));
            float dy = wrapDelta(other.y - me.y, static_cast<float>(height));
            const float d2 = dx * dx + dy * dy;
            const float d = std::sqrt(d2);
            if (d < 0.001f)
                continue;

            if (d < separationRadius)
            {
                sepX -= dx / d;
                sepY -= dy / d;
                sepCount++;
            }

            if (d < alignRadius)
            {
                aliX += other.vx;
                aliY += other.vy;
                aliCount++;
            }

            if (d < cohesionRadius)
            {
                cohX += dx;
                cohY += dy;
                cohCount++;
            }
        }

        float steerX = 0.0f;
        float steerY = 0.0f;

        if (sepCount > 0)
        {
            sepX /= sepCount;
            sepY /= sepCount;
            steerX += sepX * sepWeight;
            steerY += sepY * sepWeight;
        }

        if (aliCount > 0)
        {
            aliX /= aliCount;
            aliY /= aliCount;
            float m = std::sqrt(aliX * aliX + aliY * aliY);
            if (m > 0.0001f)
            {
                aliX = (aliX / m) * maxSpeed - me.vx;
                aliY = (aliY / m) * maxSpeed - me.vy;
                steerX += aliX * alignWeight;
                steerY += aliY * alignWeight;
            }
        }

        if (cohCount > 0)
        {
            cohX /= cohCount;
            cohY /= cohCount;
            float m = std::sqrt(cohX * cohX + cohY * cohY);
            if (m > 0.0001f)
            {
                cohX = (cohX / m) * maxSpeed - me.vx;
                cohY = (cohY / m) * maxSpeed - me.vy;
                steerX += cohX * cohWeight;
                steerY += cohY * cohWeight;
            }
        }

        float sm = std::sqrt(steerX * steerX + steerY * steerY);

        if (mouseMode != 0)
        {
            float mdx = wrapDelta(static_cast<float>(mouseX) - me.x, static_cast<float>(width));
            float mdy = wrapDelta(static_cast<float>(mouseY) - me.y, static_cast<float>(height));
            const float md = std::sqrt(mdx * mdx + mdy * mdy);
            if (md > 1.0f && md < 260.0f)
            {
                const float sign = static_cast<float>(mouseMode);
                const float pull = (1.1f - md / 260.0f) * 0.08f * sign;
                steerX += (mdx / md) * pull;
                steerY += (mdy / md) * pull;
            }
        }

        sm = std::sqrt(steerX * steerX + steerY * steerY);
        if (sm > maxForce)
        {
            steerX = (steerX / sm) * maxForce;
            steerY = (steerY / sm) * maxForce;
        }

        ax[i] = steerX;
        ay[i] = steerY;
    }

    for (size_t i = 0; i < boids.size(); i++)
    {
        Boid &b = boids[i];

        b.vx += ax[i] * dtScale;
        b.vy += ay[i] * dtScale;

        float v = std::sqrt(b.vx * b.vx + b.vy * b.vy);
        if (v > maxSpeed)
        {
            b.vx = (b.vx / v) * maxSpeed;
            b.vy = (b.vy / v) * maxSpeed;
        }

        b.x += b.vx * dtScale;
        b.y += b.vy * dtScale;

        if (b.x < 0)
            b.x += width;
        if (b.x >= width)
            b.x -= width;
        if (b.y < 0)
            b.y += height;
        if (b.y >= height)
            b.y -= height;
    }
}

void Boids::draw(SDL_Renderer *renderer)
{
    SDL_SetRenderDrawColor(renderer, 10, 12, 18, 255);
    SDL_RenderClear(renderer);

    for (const auto &b : boids)
    {
        const SDL_Color col = colors[b.type % static_cast<int>(colors.size())];
        SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, 255);

        const float ang = std::atan2(b.vy, b.vx);
        const float c = std::cos(ang);
        const float s = std::sin(ang);

        const float len = 5.0f;
        const float halfW = 1.5f;

        const float tipX = b.x + c * len;
        const float tipY = b.y + s * len;
        const float baseX = b.x - c * len * 0.6f;
        const float baseY = b.y - s * len * 0.6f;

        const float leftX = baseX - s * halfW;
        const float leftY = baseY + c * halfW;
        const float rightX = baseX + s * halfW;
        const float rightY = baseY - c * halfW;

        SDL_RenderDrawLine(renderer, static_cast<int>(tipX), static_cast<int>(tipY), static_cast<int>(leftX), static_cast<int>(leftY));
        SDL_RenderDrawLine(renderer, static_cast<int>(leftX), static_cast<int>(leftY), static_cast<int>(rightX), static_cast<int>(rightY));
        SDL_RenderDrawLine(renderer, static_cast<int>(rightX), static_cast<int>(rightY), static_cast<int>(tipX), static_cast<int>(tipY));
    }

    SDL_RenderPresent(renderer);
}

void Boids::increaseSpeed()
{
    maxSpeed = std::min(8.0f, maxSpeed + 0.2f);
}

void Boids::decreaseSpeed()
{
    maxSpeed = std::max(1.0f, maxSpeed - 0.2f);
}

void Boids::addFlock()
{
    if (flockCount >= maxFlocks)
        return;

    flockCount++;
    rebuildColors();

    const int newType = flockCount - 1;
    for (size_t i = 0; i < boids.size(); i++)
    {
        if (i % 7 == 0)
            boids[i].type = newType;
    }
}

void Boids::removeFlock()
{
    if (flockCount <= 1)
        return;

    flockCount--;
    for (auto &b : boids)
        b.type %= flockCount;
    rebuildColors();
}

void Boids::setMouseInteraction(int x, int y, int mode)
{
    mouseX = x;
    mouseY = y;
    mouseMode = mode;
}

void Boids::clearMouseInteraction()
{
    mouseMode = 0;
}
