#include "Simulation.hpp"
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <fstream>

static SDL_Color hsvToRgb(float h, float s, float v)
{
    float c = v * s;
    float x = c * (1.0f - std::abs(std::fmod(h / 60.0f, 2.0f) - 1.0f));
    float m = v - c;
    float r = 0, g = 0, b = 0;

    if (h < 60)
    {
        r = c;
        g = x;
        b = 0;
    }
    else if (h < 120)
    {
        r = x;
        g = c;
        b = 0;
    }
    else if (h < 180)
    {
        r = 0;
        g = c;
        b = x;
    }
    else if (h < 240)
    {
        r = 0;
        g = x;
        b = c;
    }
    else if (h < 300)
    {
        r = x;
        g = 0;
        b = c;
    }
    else
    {
        r = c;
        g = 0;
        b = x;
    }

    return {
        (Uint8)((r + m) * 255),
        (Uint8)((g + m) * 255),
        (Uint8)((b + m) * 255),
        255};
}

void Simulation::generateColors()
{
    colors.clear();
    for (int i = 0; i < numTypes; i++)
    {
        float hue = (360.0f / numTypes) * i;
        colors.push_back(hsvToRgb(hue, 0.85f, 1.0f));
    }
}

void Simulation::resizeRules(int newTypes)
{
    std::vector<std::vector<float>> newRules(newTypes, std::vector<float>(newTypes, 0.0f));
    for (int a = 0; a < newTypes; a++)
        for (int b = 0; b < newTypes; b++)
            if (a < numTypes && b < numTypes)
                newRules[a][b] = rules[a][b];
            else
                newRules[a][b] = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
    rules = newRules;
    numTypes = newTypes;
    generateColors();
}

Simulation::Simulation(int width, int height, int numParticles, int numTypes)
    : width(width), height(height), numParticles(numParticles),
      numTypes(numTypes), speedMultiplier(1.0f), trailsEnabled(false),
      mouseX(0), mouseY(0), mouseMode(0)
{
    particles.resize(numParticles);
    for (auto &p : particles)
    {
        p.x = (float)(rand() % width);
        p.y = (float)(rand() % height);
        p.vx = 0;
        p.vy = 0;
        p.type = rand() % numTypes;
    }
    rules.resize(numTypes, std::vector<float>(numTypes, 0.0f));
    randomizeRules();
    generateColors();
}

void Simulation::randomizeRules()
{
    for (int a = 0; a < numTypes; a++)
        for (int b = 0; b < numTypes; b++)
            rules[a][b] = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
}

void Simulation::increaseSpeed()
{
    speedMultiplier = std::min(speedMultiplier + 0.1f, 4.0f);
}

void Simulation::decreaseSpeed()
{
    speedMultiplier = std::max(speedMultiplier - 0.1f, 0.1f);
}

void Simulation::addType()
{
    if (numTypes >= 12)
        return;
    int newType = numTypes;    // index of the new type
    resizeRules(numTypes + 1); // expands rules, updates numTypes

    // spawn fresh particles of only the new type
    int spawnCount = numParticles / numTypes;
    for (int i = 0; i < spawnCount; i++)
    {
        Particle p;
        p.x = (float)(rand() % width);
        p.y = (float)(rand() % height);
        p.vx = 0;
        p.vy = 0;
        p.type = newType;
        particles.push_back(p);
    }
}
void Simulation::removeType()
{
    if (numTypes <= 2)
        return;
    int removing = numTypes - 1;

    // remove particles of the type being deleted
    particles.erase(
        std::remove_if(particles.begin(), particles.end(),
                       [removing](const Particle &p)
                       { return p.type == removing; }),
        particles.end());

    resizeRules(numTypes - 1);
    // existing particles are untouched, all types still valid
}

void Simulation::toggleTrails()
{
    trailsEnabled = !trailsEnabled;
}

void Simulation::savePreset(const std::string &path) const
{
    std::ofstream out(path);
    if (!out)
        return;

    out << numTypes << '\n';
    for (int a = 0; a < numTypes; a++)
    {
        for (int b = 0; b < numTypes; b++)
        {
            out << rules[a][b];
            if (b + 1 < numTypes)
                out << ' ';
        }
        out << '\n';
    }
}

bool Simulation::loadPreset(const std::string &path)
{
    std::ifstream in(path);
    if (!in)
        return false;

    int presetTypes = 0;
    in >> presetTypes;
    if (!in || presetTypes < 2 || presetTypes > 12)
        return false;

    std::vector<std::vector<float>> newRules(
        presetTypes, std::vector<float>(presetTypes, 0.0f));

    for (int a = 0; a < presetTypes; a++)
    {
        for (int b = 0; b < presetTypes; b++)
        {
            in >> newRules[a][b];
            if (!in)
                return false;
        }
    }

    rules = std::move(newRules);
    numTypes = presetTypes;
    generateColors();

    for (auto &p : particles)
        p.type = p.type % numTypes;

    return true;
}

void Simulation::setMouseInteraction(int x, int y, int mode)
{
    mouseX = x;
    mouseY = y;
    mouseMode = mode;
}

void Simulation::clearMouseInteraction()
{
    mouseMode = 0;
}

float Simulation::force(float dist, float attraction)
{
    const float beta = 0.3f;
    const float maxDist = 100.0f;
    float r = dist / maxDist;
    if (r < beta)
        return r / beta - 1.0f;
    else if (r < 1.0f)
        return attraction * (1.0f - std::abs(2.0f * r - 1.0f - beta) / (1.0f - beta));
    return 0.0f;
}

void Simulation::update()
{
    const float maxDist = 100.0f;
    const float dt = 0.016f * speedMultiplier;
    const float friction = 0.85f;

    for (auto &a : particles)
    {
        float fx = 0, fy = 0;
        for (auto &b : particles)
        {
            float dx = b.x - a.x;
            float dy = b.y - a.y;
            if (dx > width / 2)
                dx -= width;
            if (dx < -width / 2)
                dx += width;
            if (dy > height / 2)
                dy -= height;
            if (dy < -height / 2)
                dy += height;
            float dist = std::sqrt(dx * dx + dy * dy);
            if (dist < 0.01f || dist > maxDist)
                continue;
            float f = force(dist, rules[a.type][b.type]);
            fx += (dx / dist) * f;
            fy += (dy / dist) * f;
        }

        if (mouseMode != 0)
        {
            float dx = static_cast<float>(mouseX) - a.x;
            float dy = static_cast<float>(mouseY) - a.y;
            if (dx > width / 2)
                dx -= width;
            if (dx < -width / 2)
                dx += width;
            if (dy > height / 2)
                dy -= height;
            if (dy < -height / 2)
                dy += height;

            float dist = std::sqrt(dx * dx + dy * dy);
            if (dist > 1.0f && dist < 220.0f)
            {
                float sign = static_cast<float>(mouseMode);
                float f = sign * (1.2f - dist / 220.0f) * 3.5f;
                fx += (dx / dist) * f;
                fy += (dy / dist) * f;
            }
        }

        a.vx = (a.vx + fx * dt) * friction;
        a.vy = (a.vy + fy * dt) * friction;
    }

    for (auto &p : particles)
    {
        p.x += p.vx;
        p.y += p.vy;
        if (p.x < 0)
            p.x += width;
        if (p.x >= width)
            p.x -= width;
        if (p.y < 0)
            p.y += height;
        if (p.y >= height)
            p.y -= height;
    }
}

void Simulation::draw(SDL_Renderer *renderer)
{
    if (trailsEnabled)
    {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 10, 10, 15, 35);
        SDL_RenderFillRect(renderer, nullptr);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }
    else
    {
        SDL_SetRenderDrawColor(renderer, 10, 10, 15, 255);
        SDL_RenderClear(renderer);
    }

    for (auto &p : particles)
    {
        SDL_Color col = colors[p.type];
        SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, 255);
        SDL_Rect rect = {(int)p.x - 2, (int)p.y - 2, 4, 4};
        SDL_RenderFillRect(renderer, &rect);
    }

    SDL_RenderPresent(renderer);
}