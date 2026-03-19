#pragma once
#include "Particle.hpp"
#include <vector>
#include <string>
#include <SDL2/SDL.h>

class Simulation
{
public:
    Simulation(int width, int height, int numParticles, int numTypes);

    void randomizeRules();
    void update();
    void draw(SDL_Renderer *renderer);

    void increaseSpeed();
    void decreaseSpeed();
    void addType();
    void removeType();
    void toggleTrails();
    void savePreset(const std::string &path) const;
    bool loadPreset(const std::string &path);
    void setMouseInteraction(int x, int y, int mode);
    void clearMouseInteraction();

    // getters for control panel
    float getSpeed() const { return speedMultiplier; }
    int getNumTypes() const { return numTypes; }
    const std::vector<SDL_Color> &getColors() const { return colors; }
    bool getTrailsEnabled() const { return trailsEnabled; }
    int getMouseMode() const { return mouseMode; }
    int getParticleCount() const { return static_cast<int>(particles.size()); }

private:
    int width, height;
    int numTypes;
    int numParticles;
    float speedMultiplier;
    bool trailsEnabled;
    int mouseX, mouseY;
    int mouseMode; // 0=off, 1=attract, -1=repel

    std::vector<Particle> particles;
    std::vector<std::vector<float>> rules;
    std::vector<SDL_Color> colors;

    float force(float dist, float attraction);
    void generateColors();
    void resizeRules(int newTypes);
};