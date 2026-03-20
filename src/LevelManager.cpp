#include "LevelManager.hpp"

#include <utility>

LevelManager::LevelManager(int width, int height)
    : clickMode(ClickMode::Focus), status("hierarchy ready"),
      lmbHeld(false), rmbHeld(false), heldWx(0.0f), heldWy(0.0f),
      lastMouseX(width / 2), lastMouseY(height / 2)
{
    camera.setViewport(width, height);
}

void LevelManager::setRoot(std::unique_ptr<Level> root)
{
    stack.clear();
    if (!root)
        return;
    camera.snapTo(0.0f, 0.0f, root->preferredZoom());
    stack.push_back(std::move(root));
}

void LevelManager::update(float dt)
{
    if (current())
    {
        if (clickMode == ClickMode::Gravity)
        {
            if (lmbHeld)
                current()->applyMouseGravity(heldWx, heldWy, 1.0f);
            if (rmbHeld)
                current()->applyMouseGravity(heldWx, heldWy, -1.0f);
        }
        current()->update(dt);
    }
    camera.update(dt);
}

void LevelManager::draw(SDL_Renderer *renderer) const
{
    if (current())
        current()->draw(renderer, camera);
}

void LevelManager::handleMouseDown(Uint8 button, int sx, int sy)
{
    Level *lvl = current();
    if (!lvl)
        return;

    SDL_FPoint w = camera.screenToWorld(static_cast<float>(sx), static_cast<float>(sy));

    if (clickMode == ClickMode::Gravity)
    {
        if (button == SDL_BUTTON_LEFT)
        {
            lmbHeld = true;
            heldWx = w.x;
            heldWy = w.y;
            lvl->applyMouseGravity(w.x, w.y, 1.0f);
            status = "gravity attract";
        }
        else if (button == SDL_BUTTON_RIGHT)
        {
            rmbHeld = true;
            heldWx = w.x;
            heldWy = w.y;
            lvl->applyMouseGravity(w.x, w.y, -1.0f);
            status = "gravity repel";
        }
        return;
    }

    if (clickMode == ClickMode::Spawn)
    {
        if (button == SDL_BUTTON_LEFT)
        {
            lvl->spawnAt(w.x, w.y, false);
            status = "spawned star/object";
        }
        else if (button == SDL_BUTTON_RIGHT)
        {
            lvl->spawnAt(w.x, w.y, true);
            status = "spawned black hole";
        }
        return;
    }

    if (clickMode == ClickMode::Focus && button == SDL_BUTTON_LEFT)
    {
        zoomIntoAt(sx, sy);
    }
}

void LevelManager::zoomIntoAt(int sx, int sy)
{
    Level *lvl = current();
    if (!lvl)
        return;

    SimBody picked;
    if (!lvl->findBodyAtScreen(camera, sx, sy, picked))
    {
        status = "no target";
        return;
    }

    std::unique_ptr<Level> child = lvl->createChildFromBody(picked);
    if (!child)
    {
        status = "no deeper level";
        return;
    }

    status = "zoomed in";
    pushLevel(std::move(child));
}

void LevelManager::handleMouseUp(Uint8)
{
    lmbHeld = false;
    rmbHeld = false;
}

void LevelManager::handleMouseMotion(Uint32 state, int sx, int sy)
{
    if (state & SDL_BUTTON_MMASK)
    {
        int dx = sx - lastMouseX;
        int dy = sy - lastMouseY;
        camera.snapTo(camera.x - dx / camera.zoom, camera.y - dy / camera.zoom, camera.zoom);
    }
    lastMouseX = sx;
    lastMouseY = sy;

    SDL_FPoint w = camera.screenToWorld(static_cast<float>(sx), static_cast<float>(sy));
    heldWx = w.x;
    heldWy = w.y;

    if (clickMode != ClickMode::Gravity)
        return;

    Level *lvl = current();
    if (!lvl)
        return;

    if (state & SDL_BUTTON_LMASK)
    {
        lmbHeld = true;
        lvl->applyMouseGravity(w.x, w.y, 1.0f);
    }
    else if (state & SDL_BUTTON_RMASK)
    {
        rmbHeld = true;
        lvl->applyMouseGravity(w.x, w.y, -1.0f);
    }
}

void LevelManager::handleMouseWheel(int dy)
{
    const float factor = (dy > 0) ? 1.18f : 0.85f;
    camera.focusOn(camera.x, camera.y, camera.zoom * factor);
}

void LevelManager::handleKey(SDL_Keycode key)
{
    if (key == SDLK_b)
    {
        popLevel();
        return;
    }
    if (key == SDLK_f || key == SDLK_1)
    {
        setClickMode(ClickMode::Focus);
        return;
    }
    if (key == SDLK_g || key == SDLK_2)
    {
        setClickMode(ClickMode::Gravity);
        return;
    }
    if (key == SDLK_s || key == SDLK_3)
    {
        setClickMode(ClickMode::Spawn);
        return;
    }
}

void LevelManager::setClickMode(ClickMode mode)
{
    clickMode = mode;
    status = "mode: " + getClickModeName();
}

const Level *LevelManager::current() const
{
    return stack.empty() ? nullptr : stack.back().get();
}

Level *LevelManager::current()
{
    return stack.empty() ? nullptr : stack.back().get();
}

int LevelManager::getDepth() const
{
    return static_cast<int>(stack.size());
}

std::string LevelManager::getTitle() const
{
    const Level *lvl = current();
    return lvl ? lvl->getTitle() : "none";
}

std::string LevelManager::getStatus() const
{
    return status;
}

std::string LevelManager::getClickModeName() const
{
    switch (clickMode)
    {
    case ClickMode::Gravity:
        return "GRAVITY";
    case ClickMode::Spawn:
        return "SPAWN";
    case ClickMode::Focus:
    default:
        return "FOCUS";
    }
}

float LevelManager::getTimeScale() const
{
    const Level *lvl = current();
    return lvl ? lvl->getTimeScale() : 1.0f;
}

void LevelManager::increasePopulation()
{
    if (!current())
        return;
    current()->increasePopulation();
    status = "population increased";
}

void LevelManager::decreasePopulation()
{
    if (!current())
        return;
    current()->decreasePopulation();
    status = "population decreased";
}

void LevelManager::increaseTimeScale()
{
    if (!current())
        return;
    current()->increaseTimeScale();
    status = "time scale up";
}

void LevelManager::decreaseTimeScale()
{
    if (!current())
        return;
    current()->decreaseTimeScale();
    status = "time scale down";
}

void LevelManager::pushLevel(std::unique_ptr<Level> next)
{
    if (!next)
        return;

    camera.focusOn(0.0f, 0.0f, next->preferredZoom());
    stack.push_back(std::move(next));
}

void LevelManager::popLevel()
{
    if (stack.size() <= 1)
    {
        status = "already at root";
        return;
    }

    stack.pop_back();
    if (current())
    {
        camera.focusOn(0.0f, 0.0f, current()->preferredZoom());
        status = "zoomed out";
    }
}
