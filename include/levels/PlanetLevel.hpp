#pragma once

#include "Level.hpp"

#include <vector>

class PlanetLevel : public Level
{
public:
    explicit PlanetLevel(float planetMass = 900.0f);

    void update(float dt) override;
    void draw(SDL_Renderer *renderer, const Camera &camera) const override;

    LevelKind getKind() const override { return LevelKind::Planet; }
    std::string getTitle() const override { return "PLANET"; }
    float preferredZoom() const override { return 2.8f; }

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
