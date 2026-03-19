#include "AtomModel.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace
{
    constexpr float PI = 3.1415926535f;

    const char *kElementSymbols[] = {
        "?", "H", "He", "Li", "Be", "B", "C", "N", "O", "F", "Ne", "Na", "Mg", "Al", "Si", "P", "S", "Cl", "Ar", "K", "Ca",
        "Sc", "Ti", "V", "Cr", "Mn", "Fe", "Co", "Ni", "Cu", "Zn", "Ga", "Ge", "As", "Se", "Br", "Kr", "Rb", "Sr", "Y", "Zr",
        "Nb", "Mo", "Tc", "Ru", "Rh", "Pd", "Ag", "Cd", "In", "Sn", "Sb", "Te", "I", "Xe", "Cs", "Ba", "La", "Ce", "Pr", "Nd",
        "Pm", "Sm", "Eu", "Gd", "Tb", "Dy", "Ho", "Er", "Tm", "Yb", "Lu", "Hf", "Ta", "W", "Re", "Os", "Ir", "Pt", "Au", "Hg",
        "Tl", "Pb", "Bi", "Po", "At", "Rn", "Fr", "Ra", "Ac", "Th", "Pa", "U", "Np", "Pu", "Am", "Cm", "Bk", "Cf", "Es", "Fm",
        "Md", "No", "Lr", "Rf", "Db", "Sg", "Bh", "Hs", "Mt", "Ds", "Rg", "Cn", "Nh", "Fl", "Mc", "Lv", "Ts", "Og"};

    void drawFilledCircle(SDL_Renderer *renderer, int cx, int cy, int radius)
    {
        for (int y = -radius; y <= radius; y++)
        {
            const int xx = static_cast<int>(std::sqrt(radius * radius - y * y));
            SDL_RenderDrawLine(renderer, cx - xx, cy + y, cx + xx, cy + y);
        }
    }

    float assocLaguerre(int k, int alpha, float x)
    {
        if (k <= 0)
            return 1.0f;
        if (k == 1)
            return static_cast<float>(alpha + 1) - x;

        float Lkm2 = 1.0f;
        float Lkm1 = static_cast<float>(alpha + 1) - x;
        for (int n = 2; n <= k; n++)
        {
            float a = static_cast<float>(2 * n - 1 + alpha) - x;
            float b = static_cast<float>(n - 1 + alpha);
            float Ln = (a * Lkm1 - b * Lkm2) / static_cast<float>(n);
            Lkm2 = Lkm1;
            Lkm1 = Ln;
        }
        return Lkm1;
    }
}

AtomModel::AtomModel(int width, int height)
    : width(width), height(height), atomicNumber(1), orbitalMode(0), cloudN(2200), trailsEnabled(true),
      wobbleX(0.0f), wobbleY(0.0f), wobbleVx(0.0f), wobbleVy(0.0f),
      mouseX(width / 2), mouseY(height / 2), mouseMode(0),
      photonTimer(0.0f)
{
    rebuildElectronConfig();
    rebuildCloud();
}

void AtomModel::rebuildElectronConfig()
{
    static const int capacities[] = {2, 8, 18, 32, 32, 18, 8};

    shellElectrons.clear();
    int remaining = atomicNumber;
    for (int cap : capacities)
    {
        if (remaining <= 0)
            break;
        const int n = std::min(cap, remaining);
        shellElectrons.push_back(n);
        remaining -= n;
    }
}

float AtomModel::psi2(float r, float theta, int shell, int mode) const
{
    const int l = (mode == 0) ? 0 : (mode == 1 ? 1 : 2);
    const int n = std::max(l + 1, shell);
    const float a0 = 32.0f;
    const float rho = 2.0f * r / (n * a0);
    const int k = n - l - 1;

    const float L = assocLaguerre(k, 2 * l + 1, rho);
    const float radial = std::exp(-rho) * std::pow(std::max(0.0f, rho), 2.0f * l) * (L * L);

    float angular = 1.0f;
    if (l == 1)
    {
        // real p_x-like slice
        float c = std::cos(theta);
        angular = c * c;
    }
    else if (l == 2)
    {
        // real d_{x^2-y^2}-like slice
        float c2 = std::cos(2.0f * theta);
        angular = c2 * c2;
    }

    return std::max(0.0f, radial * angular);
}

void AtomModel::rebuildCloud()
{
    cloud.clear();
    if (shellElectrons.empty())
        return;

    // visualize one selected orbital family at a representative shell (valence-like)
    const int l = (orbitalMode == 0) ? 0 : (orbitalMode == 1 ? 1 : 2);
    const int nVis = std::max(l + 1, static_cast<int>(shellElectrons.size()));

    const float bound = std::min(width, height) * 0.42f;
    while (static_cast<int>(cloud.size()) < cloudN)
    {
        float x = (static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f) * bound;
        float y = (static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f) * bound;
        float r = std::sqrt(x * x + y * y);
        if (r > bound)
            continue;

        float theta = std::atan2(y, x);
        float p = psi2(r, theta, nVis, orbitalMode);

        // rejection with dynamic scale; keeps clear orbital silhouettes
        float scale = (orbitalMode == 0) ? 1.2f : (orbitalMode == 1 ? 0.8f : 0.35f);
        float accept = std::min(1.0f, p * scale);
        if ((static_cast<float>(rand()) / RAND_MAX) < accept)
        {
            CloudParticle cp;
            cp.x = x;
            cp.y = y;
            cp.vx = 0.0f;
            cp.vy = 0.0f;
            cp.shellIdx = nVis;
            cloud.push_back(cp);
        }
    }
}

SDL_Color AtomModel::wavelengthToColor(float nm) const
{
    if (nm >= 640.0f)
        return {255, 80, 60, 255};
    if (nm >= 580.0f)
        return {255, 180, 60, 255};
    if (nm >= 520.0f)
        return {120, 255, 90, 255};
    if (nm >= 470.0f)
        return {80, 180, 255, 255};
    return {180, 100, 255, 255};
}

void AtomModel::emitPhoton(float x, float y)
{
    static const float balmer[] = {656.0f, 486.0f, 434.0f, 410.0f};
    const float wave = balmer[rand() % 4];
    const float ang = (static_cast<float>(rand()) / RAND_MAX) * 2.0f * PI;

    Photon p;
    p.x = x;
    p.y = y;
    p.vx = std::cos(ang) * 4.0f;
    p.vy = std::sin(ang) * 4.0f;
    p.life = 1.0f;
    p.color = wavelengthToColor(wave);
    photons.push_back(p);
}

void AtomModel::update(float dt)
{
    const float cx = width * 0.5f + wobbleX;
    const float cy = height * 0.5f + wobbleY;

    if (mouseMode != 0)
    {
        float dx = static_cast<float>(mouseX) - cx;
        float dy = static_cast<float>(mouseY) - cy;
        const float d = std::sqrt(dx * dx + dy * dy);
        if (d > 1.0f)
        {
            const float sign = static_cast<float>(mouseMode);
            const float f = sign * std::max(0.0f, 1.0f - d / 500.0f) * 220.0f * dt;
            wobbleVx += (dx / d) * f;
            wobbleVy += (dy / d) * f;
        }
    }

    wobbleVx *= 0.92f;
    wobbleVy *= 0.92f;
    wobbleX += wobbleVx * dt;
    wobbleY += wobbleVy * dt;
    wobbleX = std::clamp(wobbleX, -220.0f, 220.0f);
    wobbleY = std::clamp(wobbleY, -160.0f, 160.0f);

    const float dtN = std::max(0.2f, dt * 60.0f);
    for (auto &cp : cloud)
    {
        // Metropolis-Hastings steps to follow target density |psi|^2
        for (int k = 0; k < 2; k++)
        {
            float step = (orbitalMode == 0) ? 5.5f : (orbitalMode == 1 ? 6.5f : 7.5f);
            float nx = cp.x + (static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f) * step;
            float ny = cp.y + (static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f) * step;

            float ro = std::sqrt(cp.x * cp.x + cp.y * cp.y);
            float to = std::atan2(cp.y, cp.x);
            float rn = std::sqrt(nx * nx + ny * ny);
            float tn = std::atan2(ny, nx);

            float po = std::max(1.0e-8f, psi2(ro, to, cp.shellIdx, orbitalMode));
            float pn = std::max(0.0f, psi2(rn, tn, cp.shellIdx, orbitalMode));
            float accept = std::min(1.0f, pn / po);

            if ((static_cast<float>(rand()) / RAND_MAX) < accept)
            {
                cp.x = nx;
                cp.y = ny;
            }
        }

        // mouse interaction acts on cloud directly
        if (mouseMode != 0)
        {
            const float wx = cx + cp.x;
            const float wy = cy + cp.y;
            float dx = static_cast<float>(mouseX) - wx;
            float dy = static_cast<float>(mouseY) - wy;
            float d = std::sqrt(dx * dx + dy * dy);
            if (d > 1.0f && d < 260.0f)
            {
                float sign = static_cast<float>(mouseMode);
                float mf = sign * (1.1f - d / 260.0f) * 1.8f;
                cp.x += (dx / d) * mf * dtN;
                cp.y += (dy / d) * mf * dtN;
            }
        }

        const float bound = std::min(width, height) * 0.45f;
        const float rr = std::sqrt(cp.x * cp.x + cp.y * cp.y);
        if (rr > bound && rr > 0.01f)
        {
            cp.x = (cp.x / rr) * bound;
            cp.y = (cp.y / rr) * bound;
        }
    }

    photonTimer += dt;
    if (photonTimer > 0.45f + (static_cast<float>(rand()) / RAND_MAX) * 0.5f && shellElectrons.size() > 1)
    {
        photonTimer = 0.0f;
        emitPhoton(cx, cy);
    }

    for (auto &p : photons)
    {
        p.x += p.vx;
        p.y += p.vy;
        p.life -= dt * 0.7f;
    }
    photons.erase(
        std::remove_if(photons.begin(), photons.end(), [](const Photon &p)
                       { return p.life <= 0.0f; }),
        photons.end());
}

void AtomModel::draw(SDL_Renderer *renderer)
{
    if (trailsEnabled)
    {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 10, 12, 18, 28);
        SDL_RenderFillRect(renderer, nullptr);
    }
    else
    {
        SDL_SetRenderDrawColor(renderer, 10, 12, 18, 255);
        SDL_RenderClear(renderer);
    }

    const int cx = static_cast<int>(width * 0.5f + wobbleX);
    const int cy = static_cast<int>(height * 0.5f + wobbleY);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    if (orbitalMode == 0)
        SDL_SetRenderDrawColor(renderer, 130, 190, 255, 34);
    else if (orbitalMode == 1)
        SDL_SetRenderDrawColor(renderer, 160, 255, 170, 42);
    else
        SDL_SetRenderDrawColor(renderer, 255, 180, 120, 46);
    for (const auto &cp : cloud)
        SDL_RenderDrawPoint(renderer, cx + static_cast<int>(cp.x), cy + static_cast<int>(cp.y));

    SDL_SetRenderDrawColor(renderer, 255, 190, 90, 255);
    drawFilledCircle(renderer, cx, cy, 10);

    for (const auto &p : photons)
    {
        SDL_SetRenderDrawColor(renderer, p.color.r, p.color.g, p.color.b,
                               static_cast<Uint8>(std::clamp(p.life, 0.0f, 1.0f) * 255.0f));
        drawFilledCircle(renderer, static_cast<int>(p.x), static_cast<int>(p.y), 2);
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    SDL_RenderPresent(renderer);
}

void AtomModel::clear()
{
    atomicNumber = 1;
    orbitalMode = 0;
    wobbleX = wobbleY = wobbleVx = wobbleVy = 0.0f;
    photons.clear();
    rebuildElectronConfig();
    rebuildCloud();
}

void AtomModel::randomize()
{
    atomicNumber = 1 + rand() % 118;
    orbitalMode = rand() % 3;
    photons.clear();
    rebuildElectronConfig();
    rebuildCloud();
}

void AtomModel::nextElement()
{
    atomicNumber++;
    if (atomicNumber > 118)
        atomicNumber = 1;
    rebuildElectronConfig();
    rebuildCloud();
}

void AtomModel::prevElement()
{
    atomicNumber--;
    if (atomicNumber < 1)
        atomicNumber = 118;
    rebuildElectronConfig();
    rebuildCloud();
}

void AtomModel::cycleOrbital()
{
    orbitalMode = (orbitalMode + 1) % 3;
    rebuildCloud();
}

void AtomModel::toggleTrails()
{
    trailsEnabled = !trailsEnabled;
}

void AtomModel::setMouseInteraction(int x, int y, int mode)
{
    mouseX = x;
    mouseY = y;
    mouseMode = mode;
}

void AtomModel::clearMouseInteraction()
{
    mouseMode = 0;
}

std::string AtomModel::getElementSymbol() const
{
    return kElementSymbols[std::clamp(atomicNumber, 1, 118)];
}

std::string AtomModel::getOrbitalName() const
{
    if (orbitalMode == 0)
        return "s";
    if (orbitalMode == 1)
        return "p";
    return "d";
}
