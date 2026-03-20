#pragma once

#include "Level.hpp"

#include <vector>

class GalaxyLevel : public Level
{
public:
    explicit GalaxyLevel(float seedMass = 1.0f);

    void update(float dt) override;
    void draw(SDL_Renderer *renderer, const Camera &camera) const override;

    LevelKind getKind() const override { return LevelKind::Galaxy; }
    std::string getTitle() const override { return "GALAXY"; }
    float preferredZoom() const override { return 0.032f; }

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
    float diskMass;
    float bulgeMass;
    float diskScale;
    float bulgeScale;
    int armCount;
    float twistFactor;
    float timeScale;
};
