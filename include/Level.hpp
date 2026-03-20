#pragma once

#include <SDL2/SDL.h>
#include <memory>
#include <string>

#include "Camera.hpp"

enum class LevelKind
{
    GalaxyMap,
    Galaxy,
    SolarSystem,
    Planet,
    BlackHole
};

enum class BodyKind
{
    Cluster,
    Star,
    Planet,
    Moon,
    BlackHole,
    Debris
};

struct SimBody
{
    float x = 0.0f;
    float y = 0.0f;
    float vx = 0.0f;
    float vy = 0.0f;
    float mass = 1.0f;
    float radius = 2.0f;
    SDL_Color color{255, 255, 255, 255};
    BodyKind kind = BodyKind::Debris;
    bool alive = true;
    int parent = -1;
};

class Level
{
public:
    virtual ~Level() = default;

    virtual void update(float dt) = 0;
    virtual void draw(SDL_Renderer *renderer, const Camera &camera) const = 0;

    virtual LevelKind getKind() const = 0;
    virtual std::string getTitle() const = 0;
    virtual float preferredZoom() const = 0;

    virtual bool findBodyAtScreen(const Camera &camera, int sx, int sy, SimBody &outBody) const = 0;
    virtual std::unique_ptr<Level> createChildFromBody(const SimBody &body) const = 0;

    virtual void applyMouseGravity(float wx, float wy, float sign) = 0;
    virtual void spawnAt(float wx, float wy, bool blackHole) = 0;

    virtual int getBodyCount() const = 0;

    virtual void increasePopulation() {}
    virtual void decreasePopulation() {}

    virtual void increaseTimeScale() {}
    virtual void decreaseTimeScale() {}
    virtual float getTimeScale() const { return 1.0f; }
};
