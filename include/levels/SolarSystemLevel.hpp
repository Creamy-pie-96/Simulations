#pragma once

#include "Level.hpp"

#include <vector>

class SolarSystemLevel : public Level
{
public:
    explicit SolarSystemLevel(float starMass = 12000.0f);

    void update(float dt) override;
    void draw(SDL_Renderer *renderer, const Camera &camera) const override;

    LevelKind getKind() const override { return LevelKind::SolarSystem; }
    std::string getTitle() const override { return "SOLAR SYSTEM"; }
    float preferredZoom() const override { return 1.05f; }

    bool findBodyAtScreen(const Camera &camera, int sx, int sy, SimBody &outBody) const override;
    std::unique_ptr<Level> createChildFromBody(const SimBody &body) const override;

    void applyMouseGravity(float wx, float wy, float sign) override;
    void spawnAt(float wx, float wy, bool blackHole) override;

    int getBodyCount() const override { return static_cast<int>(bodies.size()); }
    void increasePopulation() override;
    void decreasePopulation() override;
    void increaseTimeScale() override;
    void decreaseTimeScale() override;
    float getTimeScale() const override { return timeScale; }

private:
    std::vector<SimBody> bodies;
    float G;
    float softening;
    float timeScale;
};
