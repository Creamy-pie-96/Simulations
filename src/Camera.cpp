#include "Camera.hpp"

#include <algorithm>

void Camera::setViewport(int width, int height)
{
    viewW = width;
    viewH = height;
}

void Camera::snapTo(float wx, float wy, float newZoom)
{
    x = targetX = wx;
    y = targetY = wy;
    zoom = targetZoom = std::clamp(newZoom, 0.001f, 200.0f);
}

void Camera::focusOn(float wx, float wy, float newZoom)
{
    targetX = wx;
    targetY = wy;
    targetZoom = std::clamp(newZoom, 0.001f, 200.0f);
}

void Camera::update(float dt)
{
    const float k = std::clamp(dt * 8.0f, 0.0f, 1.0f);
    x += (targetX - x) * k;
    y += (targetY - y) * k;
    zoom += (targetZoom - zoom) * k;
}

SDL_FPoint Camera::worldToScreen(float wx, float wy) const
{
    SDL_FPoint p;
    p.x = (wx - x) * zoom + viewW * 0.5f;
    p.y = (wy - y) * zoom + viewH * 0.5f;
    return p;
}

SDL_FPoint Camera::screenToWorld(float sx, float sy) const
{
    SDL_FPoint p;
    p.x = (sx - viewW * 0.5f) / zoom + x;
    p.y = (sy - viewH * 0.5f) / zoom + y;
    return p;
}
