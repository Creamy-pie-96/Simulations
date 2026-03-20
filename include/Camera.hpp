#pragma once

#include <SDL2/SDL.h>

struct Camera
{
    float x = 0.0f;
    float y = 0.0f;
    float zoom = 1.0f;

    float targetX = 0.0f;
    float targetY = 0.0f;
    float targetZoom = 1.0f;

    int viewW = 1280;
    int viewH = 720;

    void setViewport(int width, int height);
    void snapTo(float wx, float wy, float newZoom);
    void focusOn(float wx, float wy, float newZoom);
    void update(float dt);

    SDL_FPoint worldToScreen(float wx, float wy) const;
    SDL_FPoint screenToWorld(float sx, float sy) const;
};
