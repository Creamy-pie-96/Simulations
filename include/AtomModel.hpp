#pragma once

#include <SDL2/SDL.h>
#include <string>
#include <vector>

class AtomModel
{
public:
    AtomModel(int width, int height);

    void update(float dt);
    void draw(SDL_Renderer *renderer);
    void clear();
    void randomize();

    void nextElement();
    void prevElement();
    void cycleOrbital();
    void toggleTrails();

    void setMouseInteraction(int x, int y, int mode);
    void clearMouseInteraction();

    int getAtomicNumber() const { return atomicNumber; }
    std::string getElementSymbol() const;
    int getElectronCount() const { return atomicNumber; }
    int getShellCount() const { return static_cast<int>(shellElectrons.size()); }
    std::string getOrbitalName() const;
    int getPhotonCount() const { return static_cast<int>(photons.size()); }
    bool getTrailsEnabled() const { return trailsEnabled; }
    std::string getModeName() const { return "ATOM MODEL"; }

private:
    struct CloudParticle
    {
        float x;
        float y;
        float vx;
        float vy;
        int shellIdx;
    };

    struct Photon
    {
        float x;
        float y;
        float vx;
        float vy;
        float life;
        SDL_Color color;
    };

    int width;
    int height;
    int atomicNumber;
    int orbitalMode; // 0=s,1=p,2=d
    int cloudN;
    bool trailsEnabled;

    std::vector<int> shellElectrons;
    std::vector<CloudParticle> cloud;

    float wobbleX;
    float wobbleY;
    float wobbleVx;
    float wobbleVy;

    int mouseX;
    int mouseY;
    int mouseMode; // 0 idle, 1 attract, -1 repel

    float photonTimer;
    std::vector<Photon> photons;

    void rebuildElectronConfig();
    void rebuildCloud();
    float psi2(float r, float theta, int shell, int orbitalMode) const;
    void emitPhoton(float x, float y);
    SDL_Color wavelengthToColor(float wavelengthNm) const;
};
