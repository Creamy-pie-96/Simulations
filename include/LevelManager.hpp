#pragma once

#include <memory>
#include <string>
#include <vector>

#include <SDL2/SDL.h>

#include "Camera.hpp"
#include "Level.hpp"

enum class ClickMode
{
    Gravity,
    Spawn,
    Focus
};

class LevelManager
{
public:
    LevelManager(int width, int height);

    void setRoot(std::unique_ptr<Level> root);

    void update(float dt);
    void draw(SDL_Renderer *renderer) const;

    void handleMouseDown(Uint8 button, int sx, int sy);
    void handleMouseUp(Uint8 button);
    void handleMouseMotion(Uint32 state, int sx, int sy);
    void handleMouseWheel(int dy);
    void handleKey(SDL_Keycode key);
    void zoomIntoAt(int sx, int sy);

    void setClickMode(ClickMode mode);

    const Level *current() const;
    Level *current();

    int getDepth() const;
    std::string getTitle() const;
    std::string getStatus() const;
    std::string getClickModeName() const;
    float getTimeScale() const;

    void increasePopulation();
    void decreasePopulation();
    void increaseTimeScale();
    void decreaseTimeScale();

private:
    std::vector<std::unique_ptr<Level>> stack;
    Camera camera;
    ClickMode clickMode;
    std::string status;

    bool lmbHeld;
    bool rmbHeld;
    float heldWx;
    float heldWy;
    int lastMouseX;
    int lastMouseY;

    void pushLevel(std::unique_ptr<Level> next);
    void popLevel();
};
