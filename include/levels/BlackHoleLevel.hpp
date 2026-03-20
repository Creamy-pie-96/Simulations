#pragma once

#include "Level.hpp"

#include <vector>

class BlackHoleLevel : public Level
{
public:
    BlackHoleLevel();

    void update(float dt) override;
    void draw(SDL_Renderer *renderer, const Camera &camera) const override;

    LevelKind getKind() const override { return LevelKind::BlackHole; }
    std::string getTitle() const override { return "BLACK HOLE"; }
    float preferredZoom() const override { return 2.0f; }

    bool findBodyAtScreen(const Camera &camera, int sx, int sy, SimBody &outBody) const override;
    std::unique_ptr<Level> createChildFromBody(const SimBody &body) const override;

    void applyMouseGravity(float wx, float wy, float sign) override;
    void spawnAt(float wx, float wy, bool blackHole) override;

    int getBodyCount() const override;
    void increasePopulation() override;
    void decreasePopulation() override;
    void increaseTimeScale() override;
    void decreaseTimeScale() override;
    float getTimeScale() const override { return timeScale; }

private:
    struct AccretionParticle
    {
        float x;
        float y;
        float vx;
        float vy;
        float temperature;
        bool alive;
    };

    std::vector<SimBody> bodies;
    std::vector<AccretionParticle> disk;
    float G;
    float softening;
    float timeScale;
    float jetTimer;
};
