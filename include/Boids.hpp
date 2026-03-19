#pragma once

#include <SDL2/SDL.h>
#include <vector>

class Boids
{
public:
    Boids(int width, int height, int count = 200);

    void update(float dt);
    void draw(SDL_Renderer *renderer);
    void clear();
    void randomize();

    void increaseSpeed();
    void decreaseSpeed();
    void addFlock();
    void removeFlock();
    void setMouseInteraction(int x, int y, int mode);
    void clearMouseInteraction();

    int getBoidCount() const { return static_cast<int>(boids.size()); }
    int getFlockCount() const { return flockCount; }
    float getMaxSpeed() const { return maxSpeed; }
    int getMaxFlocks() const { return maxFlocks; }

private:
    struct Boid
    {
        float x;
        float y;
        float vx;
        float vy;
        int type;
    };

    int width;
    int height;
    int flockCount;
    int maxFlocks;
    int mouseX;
    int mouseY;
    int mouseMode; // 0=off, 1=attract, -1=repel

    float separationRadius;
    float alignRadius;
    float cohesionRadius;

    float sepWeight;
    float alignWeight;
    float cohWeight;

    float maxSpeed;
    float maxForce;

    std::vector<Boid> boids;
    std::vector<SDL_Color> colors;

    float wrapDelta(float d, float size) const;
    void rebuildColors();
};
