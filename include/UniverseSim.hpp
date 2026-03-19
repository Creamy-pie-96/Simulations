#pragma once

#include <SDL2/SDL.h>
#include <string>
#include <vector>

class UniverseSim
{
public:
    UniverseSim(int width, int height);

    void update(float dt);
    void draw(SDL_Renderer *renderer);
    void clear();
    void randomize();

    void increaseSpeed();
    void decreaseSpeed();
    void toggleTrails();

    void spawnBlackHoleAt(int px, int py);
    void spawnStarAt(int px, int py);
    void removeNearestBody(int px, int py);

    void setMouseInteraction(int x, int y, int mode);
    void clearMouseInteraction();

    int getBodyCount() const { return static_cast<int>(bodies.size()); }
    int getBlackHoleCount() const;
    int getStarCount() const;
    float getSpeed() const { return timeScale; }
    bool getTrailsEnabled() const { return trailsEnabled; }
    std::string getModeName() const { return "UNIVERSE SIM"; }

private:
    struct Body
    {
        float x;
        float y;
        float vx;
        float vy;
        float mass;
        float radius;
        SDL_Color color;
        int kind; // 0=particle,1=star,2=blackhole
        bool alive;
    };

    int width;
    int height;

    float G;
    float softening;
    float timeScale;
    int bodyCap;
    bool trailsEnabled;

    int mouseX;
    int mouseY;
    int mouseMode; // 0 idle, 1 attract, -1 repel

    std::vector<Body> bodies;

    void spawnGalaxy();
    void checkMergers();
};
