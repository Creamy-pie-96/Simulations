#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include "AtomModel.hpp"
#include "Boids.hpp"
#include "GameOfLife.hpp"
#include "LangtonsAnt.hpp"
#include "LevelManager.hpp"
#include "MazeArt.hpp"
#include "ReactionDiffusion.hpp"
#include "Simulation.hpp"
#include "UniverseSim.hpp"
#include "levels/GalaxyMapLevel.hpp"
#include <cstdlib>
#include <ctime>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>

static SDL_Window *simWin = nullptr;
static SDL_Window *panWin = nullptr;

enum class AppMode
{
    ParticleLife = 0,
    GameOfLife,
    LangtonsAnt,
    ReactionDiffusion,
    Boids,
    AtomModel,
    UniverseSim,
    HierarchySim,
    MazeArt,
    Count
};

static AppMode nextMode(AppMode mode)
{
    int m = static_cast<int>(mode);
    m = (m + 1) % static_cast<int>(AppMode::Count);
    return static_cast<AppMode>(m);
}

static const char *modeName(AppMode mode)
{
    switch (mode)
    {
    case AppMode::ParticleLife:
        return "PARTICLE LIFE";
    case AppMode::GameOfLife:
        return "GAME OF LIFE";
    case AppMode::LangtonsAnt:
        return "LANGTON'S ANT";
    case AppMode::ReactionDiffusion:
        return "REACTION DIFFUSION";
    case AppMode::Boids:
        return "BOIDS";
    case AppMode::AtomModel:
        return "ATOM MODEL";
    case AppMode::UniverseSim:
        return "UNIVERSE SIM";
    case AppMode::HierarchySim:
        return "HIERARCHY SIM";
    case AppMode::MazeArt:
        return "MAZE ART";
    default:
        return "UNKNOWN";
    }
}

static std::string trimString(const std::string &s)
{
    size_t b = 0;
    size_t e = s.size();
    while (b < e && (s[b] == ' ' || s[b] == '\n' || s[b] == '\r' || s[b] == '\t'))
        b++;
    while (e > b && (s[e - 1] == ' ' || s[e - 1] == '\n' || s[e - 1] == '\r' || s[e - 1] == '\t'))
        e--;
    return s.substr(b, e - b);
}

static std::string browseImageFilePath()
{
    const char *commands[] = {
        "zenity --file-selection --title=\"Choose image\" --file-filter=\"Images | *.png *.jpg *.jpeg *.bmp\" 2>/dev/null",
        "kdialog --getopenfilename \"$HOME\" \"Images (*.png *.jpg *.jpeg *.bmp)\" 2>/dev/null",
        "yad --file --title=\"Choose image\" --file-filter=\"Images (*.png *.jpg *.jpeg *.bmp)\" 2>/dev/null",
        "qarma --file-selection --title=\"Choose image\" --file-filter=\"Images | *.png *.jpg *.jpeg *.bmp\" 2>/dev/null"};

    for (const char *cmd : commands)
    {
        FILE *pipe = popen(cmd, "r");
        if (!pipe)
            continue;

        char buffer[4096];
        std::string out;
        while (fgets(buffer, sizeof(buffer), pipe))
            out += buffer;
        int rc = pclose(pipe);

        out = trimString(out);
        if (rc == 0 && !out.empty())
            return out;
    }
    return "";
}

// ── draw helpers ────────────────────────────────────────────────
static void drawRect(SDL_Renderer *r, int x, int y, int w, int h,
                     Uint8 R, Uint8 G, Uint8 B)
{
    SDL_SetRenderDrawColor(r, R, G, B, 255);
    SDL_Rect rect = {x, y, w, h};
    SDL_RenderFillRect(r, &rect);
}

static void drawOutline(SDL_Renderer *r, int x, int y, int w, int h,
                        Uint8 R, Uint8 G, Uint8 B)
{
    SDL_SetRenderDrawColor(r, R, G, B, 255);
    SDL_Rect rect = {x, y, w, h};
    SDL_RenderDrawRect(r, &rect);
}

static void drawText(SDL_Renderer *r, TTF_Font *font, const std::string &text,
                     int x, int y, Uint8 R, Uint8 G, Uint8 B)
{
    SDL_Color col = {R, G, B, 255};
    SDL_Surface *surf = TTF_RenderText_Blended(font, text.c_str(), col);
    if (!surf)
        return;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(r, surf);
    SDL_Rect dst = {x, y, surf->w, surf->h};
    SDL_RenderCopy(r, tex, nullptr, &dst);
    SDL_DestroyTexture(tex);
    SDL_FreeSurface(surf);
}

static void drawBar(SDL_Renderer *r, int x, int y, int w, int h,
                    float frac, Uint8 R, Uint8 G, Uint8 B)
{
    drawRect(r, x, y, w, h, 35, 35, 48);
    drawOutline(r, x, y, w, h, 70, 70, 95);
    int fill = (int)(frac * (w - 2));
    if (fill > 0)
        drawRect(r, x + 1, y + 1, fill, h - 2, R, G, B);
}

// ── control panel ───────────────────────────────────────────────
struct PanelState
{
    AppMode mode;
    float fps;
    std::string status;
};

static void drawPanel(SDL_Renderer *r, TTF_Font *font,
                      TTF_Font *small, const Simulation &sim,
                      const GameOfLife &gol,
                      const LangtonsAnt &ant,
                      const ReactionDiffusion &rd,
                      const Boids &boids,
                      const AtomModel &atom,
                      const UniverseSim &universe,
                      const LevelManager &hier,
                      const MazeArt &maze,
                      const PanelState &state)
{
    const int PW = 280;
    const int PAD = 14;
    const int BW = PW - PAD * 2;

    drawRect(r, 0, 0, PW, 620, 15, 15, 22);

    int y = 12;

    // ── TITLE ──────────────────────────────────────────────────
    drawText(r, font, "CONTROL PANEL", PAD, y, 180, 180, 220);
    std::string fpsText = "FPS " + std::to_string((int)state.fps);
    drawText(r, small, fpsText, PW - PAD - 56, y + 2, 200, 210, 220);

    drawText(r, small, std::string("mode: ") + modeName(state.mode),
             PAD, y + 20,
             state.mode == AppMode::ParticleLife ? 100 : 120,
             state.mode == AppMode::ParticleLife ? 210 : 230,
             state.mode == AppMode::ParticleLife ? 255 : 140);

    y += 36;
    drawRect(r, PAD, y, BW, 1, 50, 50, 70);
    y += 12;

    if (state.mode == AppMode::ParticleLife)
    {
        drawText(r, small, "SPEED", PAD, y, 100, 180, 255);

        std::string speedStr = std::to_string(sim.getSpeed()).substr(0, 4) + "x";
        drawText(r, small, speedStr, PW - PAD - 40, y, 80, 180, 255);
        y += 18;

        float speedFrac = (sim.getSpeed() - 0.1f) / (4.0f - 0.1f);
        drawBar(r, PAD, y, BW, 14, speedFrac, 80, 180, 255);

        int notchX = PAD + (int)((0.9f / 3.9f) * (BW - 2));
        drawRect(r, notchX, y, 2, 14, 255, 230, 80);
        y += 22;

        drawText(r, small, "UP / DOWN to change", PAD, y, 70, 70, 95);
        y += 18;
        drawRect(r, PAD, y, BW, 1, 40, 40, 55);
        y += 12;

        std::string colLabel = "COLORS  (" +
                               std::to_string(sim.getNumTypes()) + " active)";
        drawText(r, small, colLabel, PAD, y, 255, 160, 80);
        y += 18;

        const auto &cols = sim.getColors();
        int cx = PAD;
        for (int i = 0; i < (int)cols.size(); i++)
        {
            SDL_Color c = cols[i];
            drawRect(r, cx, y, 20, 20, c.r, c.g, c.b);
            drawOutline(r, cx, y, 20, 20, 200, 200, 200);
            drawText(r, small, std::to_string(i), cx + 4, y + 4, 20, 20, 20);

            cx += 26;
            if (cx + 20 > PW - PAD)
            {
                cx = PAD;
                y += 26;
            }
        }
        y += 30;

        std::string trail = std::string("Trails: ") + (sim.getTrailsEnabled() ? "ON" : "OFF");
        drawText(r, small, trail, PAD, y, 170, 170, 220);
        y += 18;

        std::string mouse = "Mouse: ";
        if (sim.getMouseMode() > 0)
            mouse += "attract";
        else if (sim.getMouseMode() < 0)
            mouse += "repel";
        else
            mouse += "idle";
        drawText(r, small, mouse, PAD, y, 170, 170, 220);
        y += 18;

        drawRect(r, PAD, y, BW, 1, 40, 40, 55);
        y += 12;

        drawText(r, small, "Particles: " + std::to_string(sim.getParticleCount()),
                 PAD, y, 130, 210, 255);
        y += 18;

        drawText(r, small, "= add type / - remove type", PAD, y, 70, 70, 95);
        y += 16;
        drawText(r, small, "LMB attract / RMB repel", PAD, y, 70, 70, 95);
        y += 16;
        drawText(r, small, "T trails / P save / L load", PAD, y, 70, 70, 95);
        y += 16;
    }
    else if (state.mode == AppMode::GameOfLife)
    {
        drawText(r, small, "TICK RATE", PAD, y, 120, 230, 140);
        std::string tickStr = std::to_string((int)gol.getTickRate()) + " t/s";
        drawText(r, small, tickStr, PW - PAD - 46, y, 120, 230, 140);
        y += 18;

        float frac = (gol.getTickRate() - 1.0f) / 59.0f;
        drawBar(r, PAD, y, BW, 14, frac, 120, 230, 140);
        y += 22;

        drawText(r, small, "UP / DOWN to change", PAD, y, 70, 70, 95);
        y += 18;

        drawRect(r, PAD, y, BW, 1, 40, 40, 55);
        y += 12;

        drawText(r, small, "Grid: " + std::to_string(gol.getCols()) + "x" + std::to_string(gol.getRows()),
                 PAD, y, 120, 230, 140);
        y += 18;
        drawText(r, small, "Alive: " + std::to_string(gol.getAliveCount()),
                 PAD, y, 120, 230, 140);
        y += 18;
        drawText(r, small, "Fading: " + std::to_string(gol.getFadingCount()),
                 PAD, y, 120, 230, 140);
        y += 18;

        drawText(r, small, "LMB draw / RMB erase", PAD, y, 70, 70, 95);
        y += 16;
        drawText(r, small, "1 glider / 2 pulsar / 3 gun", PAD, y, 70, 70, 95);
        y += 16;
        drawText(r, small, "R random / C clear", PAD, y, 70, 70, 95);
        y += 16;
    }
    else if (state.mode == AppMode::LangtonsAnt)
    {
        drawText(r, small, "STEPS", PAD, y, 255, 190, 120);
        drawText(r, small, std::to_string((int)ant.getStepCount()), PW - PAD - 70, y, 255, 190, 120);
        y += 18;

        drawText(r, small, "ANTS", PAD, y, 255, 190, 120);
        drawText(r, small, std::to_string(ant.getAntCount()), PW - PAD - 30, y, 255, 190, 120);
        y += 18;

        drawText(r, small, "tick " + std::to_string((int)ant.getTickRate()) + " t/s", PAD, y, 255, 190, 120);
        y += 18;

        drawRect(r, PAD, y, BW, 1, 40, 40, 55);
        y += 12;
        drawText(r, small, "UP/DOWN speed", PAD, y, 70, 70, 95);
        y += 16;
        drawText(r, small, "A or LMB add ant (max 24)", PAD, y, 70, 70, 95);
        y += 16;
        drawText(r, small, "drag LMB black / RMB white", PAD, y, 70, 70, 95);
        y += 16;
        drawText(r, small, "R random / C clear", PAD, y, 70, 70, 95);
        y += 16;
    }
    else if (state.mode == AppMode::ReactionDiffusion)
    {
        drawText(r, small, "Preset", PAD, y, 120, 220, 255);
        drawText(r, small, rd.getPresetName(), PAD + 58, y, 140, 240, 255);
        y += 18;

        drawRect(r, PAD, y, BW, 1, 40, 40, 55);
        y += 12;
        drawText(r, small, "N next preset", PAD, y, 70, 70, 95);
        y += 16;
        drawText(r, small, "drag LMB inject / RMB erase", PAD, y, 70, 70, 95);
        y += 16;
        drawText(r, small, "R reseed / C clear", PAD, y, 70, 70, 95);
        y += 16;
    }
    else if (state.mode == AppMode::Boids)
    {
        drawText(r, small, "Boids: " + std::to_string(boids.getBoidCount()), PAD, y, 200, 255, 130);
        y += 18;
        drawText(r, small, "Flocks: " + std::to_string(boids.getFlockCount()) + "/" + std::to_string(boids.getMaxFlocks()), PAD, y, 200, 255, 130);
        y += 18;
        drawText(r, small, "Max speed: " + std::to_string(boids.getMaxSpeed()).substr(0, 4), PAD, y, 200, 255, 130);
        y += 18;

        drawRect(r, PAD, y, BW, 1, 40, 40, 55);
        y += 12;
        drawText(r, small, "UP/DOWN speed", PAD, y, 70, 70, 95);
        y += 16;
        drawText(r, small, "= add flock / - remove", PAD, y, 70, 70, 95);
        y += 16;
        drawText(r, small, "LMB attract / RMB repel", PAD, y, 70, 70, 95);
        y += 16;
        drawText(r, small, "R random / C clear", PAD, y, 70, 70, 95);
        y += 16;
    }
    else if (state.mode == AppMode::AtomModel)
    {
        drawText(r, small, "Element: Z=" + std::to_string(atom.getAtomicNumber()) + " (" + atom.getElementSymbol() + ")", PAD, y, 190, 220, 255);
        y += 18;
        drawText(r, small, "Electrons: " + std::to_string(atom.getElectronCount()) + "  Shells: " + std::to_string(atom.getShellCount()), PAD, y, 190, 220, 255);
        y += 18;
        drawText(r, small, "Orbital cloud: " + atom.getOrbitalName(), PAD, y, 190, 220, 255);
        y += 18;
        drawText(r, small, "Photons: " + std::to_string(atom.getPhotonCount()), PAD, y, 190, 220, 255);
        y += 18;

        drawRect(r, PAD, y, BW, 1, 40, 40, 55);
        y += 12;
        drawText(r, small, "LEFT/RIGHT element", PAD, y, 70, 70, 95);
        y += 16;
        drawText(r, small, "O change orbital cloud", PAD, y, 70, 70, 95);
        y += 16;
        drawText(r, small, "LMB attract / RMB repel", PAD, y, 70, 70, 95);
        y += 16;
        drawText(r, small, "T trails", PAD, y, 70, 70, 95);
        y += 16;
        drawText(r, small, "R random / C reset H", PAD, y, 70, 70, 95);
        y += 16;
    }
    else if (state.mode == AppMode::UniverseSim)
    {
        drawText(r, small, "Bodies: " + std::to_string(universe.getBodyCount()), PAD, y, 170, 220, 255);
        y += 18;
        drawText(r, small, "Stars: " + std::to_string(universe.getStarCount()), PAD, y, 170, 220, 255);
        y += 18;
        drawText(r, small, "Black holes: " + std::to_string(universe.getBlackHoleCount()), PAD, y, 170, 220, 255);
        y += 18;
        drawText(r, small, "Time scale: " + std::to_string(universe.getSpeed()).substr(0, 4), PAD, y, 170, 220, 255);
        y += 18;

        drawRect(r, PAD, y, BW, 1, 40, 40, 55);
        y += 12;
        drawText(r, small, "UP/DOWN gravity speed", PAD, y, 70, 70, 95);
        y += 16;
        drawText(r, small, "LMB black hole / RMB star", PAD, y, 70, 70, 95);
        y += 16;
        drawText(r, small, "MMB remove nearest", PAD, y, 70, 70, 95);
        y += 16;
        drawText(r, small, "drag LMB attract / RMB repel", PAD, y, 70, 70, 95);
        y += 16;
        drawText(r, small, "T trails", PAD, y, 70, 70, 95);
        y += 16;
        drawText(r, small, "R new galaxy / C clear", PAD, y, 70, 70, 95);
        y += 16;
    }
    else if (state.mode == AppMode::HierarchySim)
    {
        drawText(r, small, "Level: " + hier.getTitle(), PAD, y, 170, 220, 255);
        y += 18;
        drawText(r, small, "Depth: " + std::to_string(hier.getDepth()), PAD, y, 170, 220, 255);
        y += 18;
        if (hier.current())
        {
            drawText(r, small, "Bodies: " + std::to_string(hier.current()->getBodyCount()), PAD, y, 170, 220, 255);
            y += 18;
        }
        drawText(r, small, "Click mode: " + hier.getClickModeName(), PAD, y, 170, 220, 255);
        y += 18;
        drawText(r, small, "Time scale: " + std::to_string(hier.getTimeScale()).substr(0, 4), PAD, y, 170, 220, 255);
        y += 18;

        drawRect(r, PAD, y, BW, 1, 40, 40, 55);
        y += 12;
        drawText(r, small, "F/1 Focus, G/2 Gravity", PAD, y, 70, 70, 95);
        y += 16;
        drawText(r, small, "S/3 Spawn, B zoom out", PAD, y, 70, 70, 95);
        y += 16;
        drawText(r, small, "UP/DOWN time, =/- objects", PAD, y, 70, 70, 95);
        y += 16;
        drawText(r, small, "Focus: LMB or Z on target", PAD, y, 70, 70, 95);
        y += 16;
        drawText(r, small, "Gravity: hold LMB/RMB", PAD, y, 70, 70, 95);
        y += 16;
        drawText(r, small, "Spawn: LMB star, RMB BH", PAD, y, 70, 70, 95);
        y += 16;
        drawText(r, small, "Wheel zoom, MMB drag pan", PAD, y, 70, 70, 95);
        y += 16;
    }
    else if (state.mode == AppMode::MazeArt)
    {
        drawText(r, small, "Visited: " + std::to_string(maze.getVisitedCount()) + "/" + std::to_string(maze.getTotalCells()), PAD, y, 190, 220, 255);
        y += 18;
        drawText(r, small, "Grid: " + std::to_string(maze.getCols()) + "x" + std::to_string(maze.getRows()), PAD, y, 190, 220, 255);
        y += 18;
        drawText(r, small, "Speed: " + std::to_string(maze.getSpeed()).substr(0, 4) + " steps/frame", PAD, y, 190, 220, 255);
        y += 18;
        drawText(r, small, std::string("Image: ") + (maze.hasImage() ? "loaded" : "none"), PAD, y, 190, 220, 255);
        y += 18;
        drawText(r, small, std::string("Monochrome: ") + (maze.getMonochrome() ? "ON" : "OFF"), PAD, y, 190, 220, 255);
        y += 18;

        drawRect(r, PAD, y, BW, 1, 40, 40, 55);
        y += 12;
        drawText(r, small, "UP/DOWN speed", PAD, y, 70, 70, 95);
        y += 16;
        drawText(r, small, "R regenerate", PAD, y, 70, 70, 95);
        y += 16;
        drawText(r, small, "I browse/load (png/jpg/jpeg/bmp)", PAD, y, 70, 70, 95);
        y += 16;
        drawText(r, small, "U unload image (perfect maze)", PAD, y, 70, 70, 95);
        y += 16;
        drawText(r, small, "K toggle monochrome", PAD, y, 70, 70, 95);
        y += 16;
    }

    drawRect(r, PAD, y, BW, 1, 40, 40, 55);
    y += 12;

    drawText(r, small, "M switch modes / ESC quit", PAD, y, 150, 255, 150);
    y += 18;
    drawText(r, small, state.status, PAD, y, 240, 180, 120);

    SDL_RenderPresent(r);
}

// ────────────────────────────────────────────────────────────────
int main()
{
    srand(time(nullptr));

    const int SIM_W = 1280, SIM_H = 720;
    const int PAN_W = 280, PAN_H = 620;

    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
    TTF_Init();

    // load a system monospace font
    TTF_Font *font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf", 14);
    TTF_Font *small = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf", 11);

    if (!font || !small)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Font error",
                                 TTF_GetError(), nullptr);
        return 1;
    }

    simWin = SDL_CreateWindow(
        "Particle Life",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SIM_W, SIM_H, 0);
    SDL_Renderer *simRen = SDL_CreateRenderer(
        simWin, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_SetRenderDrawBlendMode(simRen, SDL_BLENDMODE_BLEND);

    int simX, simY;
    SDL_GetWindowPosition(simWin, &simX, &simY);

    panWin = SDL_CreateWindow(
        "Control Panel",
        simX - PAN_W - 10, simY,
        PAN_W, PAN_H, 0);
    SDL_Renderer *panRen = SDL_CreateRenderer(panWin, -1, SDL_RENDERER_ACCELERATED);

    Uint32 simID = SDL_GetWindowID(simWin);
    Uint32 panID = SDL_GetWindowID(panWin);

    Simulation sim(SIM_W, SIM_H, 600, 5);
    GameOfLife gol(SIM_W, SIM_H, 10);
    LangtonsAnt ant(SIM_W, SIM_H, 8);
    ReactionDiffusion rd(SIM_W, SIM_H, 3);
    Boids boids(SIM_W, SIM_H, 220);
    AtomModel atom(SIM_W, SIM_H);
    UniverseSim universe(SIM_W, SIM_H);
    LevelManager hierarchy(SIM_W, SIM_H);
    MazeArt maze(SIM_W, SIM_H, 8);
    hierarchy.setRoot(std::make_unique<GalaxyMapLevel>());
    gol.randomize(0.17f);

    AppMode mode = AppMode::ParticleLife;

    bool running = true;
    SDL_Event event;
    Uint32 lastThrottle = 0;
    const Uint32 THROTTLE_MS = 120;
    Uint32 lastFrame = SDL_GetTicks();
    Uint32 fpsTimer = lastFrame;
    int fpsFrames = 0;
    float fps = 0.0f;
    std::string status = "ready";

    std::string presetPath = "particle_rules.preset";
    char *prefPathRaw = SDL_GetPrefPath("ParticleLife", "ParticleLife");
    if (prefPathRaw)
    {
        presetPath = std::string(prefPathRaw) + "particle_rules.preset";
        SDL_free(prefPathRaw);
    }
    std::cout << "[ParticleLife] preset path: " << presetPath << '\n';

    while (running)
    {
        Uint32 nowTicks = SDL_GetTicks();
        float dt = (nowTicks - lastFrame) / 1000.0f;
        lastFrame = nowTicks;

        fpsFrames++;
        if (nowTicks - fpsTimer >= 500)
        {
            fps = fpsFrames * 1000.0f / (nowTicks - fpsTimer);
            fpsFrames = 0;
            fpsTimer = nowTicks;
        }

        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                running = false;
            }
            if (event.type == SDL_WINDOWEVENT &&
                event.window.event == SDL_WINDOWEVENT_CLOSE)
            {
                if (event.window.windowID == simID ||
                    event.window.windowID == panID)
                    running = false;
            }

            if (event.type == SDL_MOUSEBUTTONUP && event.button.windowID == simID)
            {
                sim.clearMouseInteraction();
                boids.clearMouseInteraction();
                atom.clearMouseInteraction();
                universe.clearMouseInteraction();
                if (mode == AppMode::HierarchySim)
                    hierarchy.handleMouseUp(event.button.button);
            }

            if (event.type == SDL_MOUSEMOTION && event.motion.windowID == simID)
            {
                if (mode == AppMode::ParticleLife)
                {
                    if (event.motion.state & SDL_BUTTON_LMASK)
                        sim.setMouseInteraction(event.motion.x, event.motion.y, 1);
                    else if (event.motion.state & SDL_BUTTON_RMASK)
                        sim.setMouseInteraction(event.motion.x, event.motion.y, -1);
                }
                else if (mode == AppMode::GameOfLife)
                {
                    if (event.motion.state & SDL_BUTTON_LMASK)
                        gol.paintCell(event.motion.x, event.motion.y, true);
                    if (event.motion.state & SDL_BUTTON_RMASK)
                        gol.paintCell(event.motion.x, event.motion.y, false);
                }
                else if (mode == AppMode::LangtonsAnt)
                {
                    if (event.motion.state & SDL_BUTTON_LMASK)
                        ant.paintCell(event.motion.x, event.motion.y, true);
                    if (event.motion.state & SDL_BUTTON_RMASK)
                        ant.paintCell(event.motion.x, event.motion.y, false);
                }
                else if (mode == AppMode::ReactionDiffusion)
                {
                    if (event.motion.state & SDL_BUTTON_LMASK)
                        rd.injectAtPixel(event.motion.x, event.motion.y);
                    if (event.motion.state & SDL_BUTTON_RMASK)
                        rd.eraseAtPixel(event.motion.x, event.motion.y);
                }
                else if (mode == AppMode::Boids)
                {
                    if (event.motion.state & SDL_BUTTON_LMASK)
                        boids.setMouseInteraction(event.motion.x, event.motion.y, 1);
                    else if (event.motion.state & SDL_BUTTON_RMASK)
                        boids.setMouseInteraction(event.motion.x, event.motion.y, -1);
                }
                else if (mode == AppMode::AtomModel)
                {
                    if (event.motion.state & SDL_BUTTON_LMASK)
                        atom.setMouseInteraction(event.motion.x, event.motion.y, 1);
                    else if (event.motion.state & SDL_BUTTON_RMASK)
                        atom.setMouseInteraction(event.motion.x, event.motion.y, -1);
                }
                else if (mode == AppMode::UniverseSim)
                {
                    if (event.motion.state & SDL_BUTTON_LMASK)
                        universe.setMouseInteraction(event.motion.x, event.motion.y, 1);
                    else if (event.motion.state & SDL_BUTTON_RMASK)
                        universe.setMouseInteraction(event.motion.x, event.motion.y, -1);
                }
                else if (mode == AppMode::HierarchySim)
                {
                    hierarchy.handleMouseMotion(event.motion.state, event.motion.x, event.motion.y);
                }
            }

            if (event.type == SDL_MOUSEWHEEL)
            {
                int mx = 0, my = 0;
                Uint32 win = SDL_GetMouseState(&mx, &my);
                (void)win;
                if (mode == AppMode::HierarchySim)
                {
                    hierarchy.handleMouseWheel(event.wheel.y);
                    status = hierarchy.getStatus();
                }
            }

            if (event.type == SDL_MOUSEBUTTONDOWN && event.button.windowID == simID)
            {
                if (mode == AppMode::ParticleLife)
                {
                    if (event.button.button == SDL_BUTTON_LEFT)
                        sim.setMouseInteraction(event.button.x, event.button.y, 1);
                    else if (event.button.button == SDL_BUTTON_RIGHT)
                        sim.setMouseInteraction(event.button.x, event.button.y, -1);
                }
                else if (mode == AppMode::GameOfLife)
                {
                    if (event.button.button == SDL_BUTTON_LEFT)
                        gol.paintCell(event.button.x, event.button.y, true);
                    if (event.button.button == SDL_BUTTON_RIGHT)
                        gol.paintCell(event.button.x, event.button.y, false);
                }
                else if (mode == AppMode::LangtonsAnt)
                {
                    if (event.button.button == SDL_BUTTON_LEFT)
                    {
                        ant.addAntAtPixel(event.button.x, event.button.y);
                        ant.paintCell(event.button.x, event.button.y, true);
                    }
                    else if (event.button.button == SDL_BUTTON_RIGHT)
                    {
                        ant.paintCell(event.button.x, event.button.y, false);
                    }
                }
                else if (mode == AppMode::ReactionDiffusion)
                {
                    if (event.button.button == SDL_BUTTON_LEFT)
                        rd.injectAtPixel(event.button.x, event.button.y);
                    else if (event.button.button == SDL_BUTTON_RIGHT)
                        rd.eraseAtPixel(event.button.x, event.button.y);
                }
                else if (mode == AppMode::Boids)
                {
                    if (event.button.button == SDL_BUTTON_LEFT)
                        boids.setMouseInteraction(event.button.x, event.button.y, 1);
                    else if (event.button.button == SDL_BUTTON_RIGHT)
                        boids.setMouseInteraction(event.button.x, event.button.y, -1);
                }
                else if (mode == AppMode::AtomModel)
                {
                    if (event.button.button == SDL_BUTTON_LEFT)
                        atom.setMouseInteraction(event.button.x, event.button.y, 1);
                    else if (event.button.button == SDL_BUTTON_RIGHT)
                        atom.setMouseInteraction(event.button.x, event.button.y, -1);
                }
                else if (mode == AppMode::UniverseSim)
                {
                    if (event.button.button == SDL_BUTTON_LEFT)
                    {
                        universe.spawnBlackHoleAt(event.button.x, event.button.y);
                        universe.setMouseInteraction(event.button.x, event.button.y, 1);
                        status = "black hole spawned";
                    }
                    else if (event.button.button == SDL_BUTTON_RIGHT)
                    {
                        universe.spawnStarAt(event.button.x, event.button.y);
                        universe.setMouseInteraction(event.button.x, event.button.y, -1);
                        status = "star spawned";
                    }
                    else if (event.button.button == SDL_BUTTON_MIDDLE)
                    {
                        universe.removeNearestBody(event.button.x, event.button.y);
                        status = "nearest body removed";
                    }
                }
                else if (mode == AppMode::HierarchySim)
                {
                    hierarchy.handleMouseDown(event.button.button, event.button.x, event.button.y);
                    status = hierarchy.getStatus();
                }
            }

            if (event.type == SDL_KEYDOWN)
            {
                switch (event.key.keysym.sym)
                {
                case SDLK_m:
                    mode = nextMode(mode);
                    status = std::string("switched to ") + modeName(mode);
                    sim.clearMouseInteraction();
                    boids.clearMouseInteraction();
                    atom.clearMouseInteraction();
                    universe.clearMouseInteraction();
                    break;
                case SDLK_ESCAPE:
                    running = false;
                    break;
                default:
                    break;
                }

                if (mode == AppMode::HierarchySim)
                {
                    hierarchy.handleKey(event.key.keysym.sym);
                    status = hierarchy.getStatus();
                }

                if (event.key.keysym.sym == SDLK_r)
                {
                    if (mode == AppMode::ParticleLife)
                    {
                        sim.randomizeRules();
                        status = "particle rules randomized";
                    }
                    else if (mode == AppMode::GameOfLife)
                    {
                        gol.randomize(0.17f);
                        status = "GoL randomized";
                    }
                    else if (mode == AppMode::LangtonsAnt)
                    {
                        ant.randomize();
                        status = "ants randomized";
                    }
                    else if (mode == AppMode::ReactionDiffusion)
                    {
                        rd.randomize();
                        status = "RD reseeded";
                    }
                    else if (mode == AppMode::Boids)
                    {
                        boids.randomize();
                        status = "boids randomized";
                    }
                    else if (mode == AppMode::AtomModel)
                    {
                        atom.randomize();
                        status = "atom randomized";
                    }
                    else if (mode == AppMode::UniverseSim)
                    {
                        universe.randomize();
                        status = "new galaxy seeded";
                    }
                    else if (mode == AppMode::HierarchySim)
                    {
                        hierarchy.setRoot(std::make_unique<GalaxyMapLevel>());
                        status = "hierarchy reset";
                    }
                    else if (mode == AppMode::MazeArt)
                    {
                        maze.regenerate(false);
                        status = "maze regenerated";
                    }
                }

                if (event.key.keysym.sym == SDLK_c)
                {
                    if (mode == AppMode::GameOfLife)
                    {
                        gol.clear();
                        status = "GoL cleared";
                    }
                    else if (mode == AppMode::LangtonsAnt)
                    {
                        ant.clear();
                        status = "ants cleared";
                    }
                    else if (mode == AppMode::ReactionDiffusion)
                    {
                        rd.clear();
                        status = "RD cleared";
                    }
                    else if (mode == AppMode::Boids)
                    {
                        boids.clear();
                        status = "boids cleared";
                    }
                    else if (mode == AppMode::AtomModel)
                    {
                        atom.clear();
                        status = "atom reset to Hydrogen";
                    }
                    else if (mode == AppMode::UniverseSim)
                    {
                        universe.clear();
                        status = "universe cleared";
                    }
                    else if (mode == AppMode::HierarchySim)
                    {
                        hierarchy.setRoot(std::make_unique<GalaxyMapLevel>());
                        status = "hierarchy reset";
                    }
                    else if (mode == AppMode::MazeArt)
                    {
                        maze.unloadImage();
                        status = "maze image unloaded";
                    }
                }

                if (mode == AppMode::ParticleLife)
                {
                    if (event.key.keysym.sym == SDLK_t)
                    {
                        sim.toggleTrails();
                        status = sim.getTrailsEnabled() ? "trails enabled" : "trails disabled";
                    }
                    else if (event.key.keysym.sym == SDLK_p)
                    {
                        sim.savePreset(presetPath);
                        status = "saved preset -> " + presetPath;
                        std::cout << "[ParticleLife] saved preset: " << presetPath << '\n';
                    }
                    else if (event.key.keysym.sym == SDLK_l)
                    {
                        bool ok = sim.loadPreset(presetPath);
                        status = ok ? "loaded preset <- " + presetPath : "failed to load preset";
                        std::cout << "[ParticleLife] load preset " << (ok ? "ok: " : "failed: ") << presetPath << '\n';
                    }
                }
                else if (mode == AppMode::GameOfLife)
                {
                    if (event.key.keysym.sym == SDLK_1)
                    {
                        int mx = SIM_W / 2, my = SIM_H / 2;
                        SDL_GetMouseState(&mx, &my);
                        gol.placePattern("glider", mx, my);
                        status = "placed glider";
                    }
                    else if (event.key.keysym.sym == SDLK_2)
                    {
                        int mx = SIM_W / 2, my = SIM_H / 2;
                        SDL_GetMouseState(&mx, &my);
                        gol.placePattern("pulsar", mx, my);
                        status = "placed pulsar";
                    }
                    else if (event.key.keysym.sym == SDLK_3)
                    {
                        int mx = SIM_W / 2, my = SIM_H / 2;
                        SDL_GetMouseState(&mx, &my);
                        gol.placePattern("gosper", mx, my);
                        status = "placed Gosper gun";
                    }
                }
                else if (mode == AppMode::LangtonsAnt)
                {
                    if (event.key.keysym.sym == SDLK_a)
                    {
                        ant.addAnt();
                        status = "added ant";
                    }
                }
                else if (mode == AppMode::ReactionDiffusion)
                {
                    if (event.key.keysym.sym == SDLK_n)
                    {
                        rd.nextPreset();
                        status = "preset: " + rd.getPresetName();
                    }
                }
                else if (mode == AppMode::AtomModel)
                {
                    if (event.key.keysym.sym == SDLK_RIGHT)
                    {
                        atom.nextElement();
                        status = "atom Z=" + std::to_string(atom.getAtomicNumber()) + " (" + atom.getElementSymbol() + ")";
                    }
                    else if (event.key.keysym.sym == SDLK_LEFT)
                    {
                        atom.prevElement();
                        status = "atom Z=" + std::to_string(atom.getAtomicNumber()) + " (" + atom.getElementSymbol() + ")";
                    }
                    else if (event.key.keysym.sym == SDLK_o)
                    {
                        atom.cycleOrbital();
                        status = "orbital cloud: " + atom.getOrbitalName();
                    }
                    else if (event.key.keysym.sym == SDLK_t)
                    {
                        atom.toggleTrails();
                        status = std::string("atom trails ") + (atom.getTrailsEnabled() ? "enabled" : "disabled");
                    }
                }
                else if (mode == AppMode::UniverseSim)
                {
                    if (event.key.keysym.sym == SDLK_t)
                    {
                        universe.toggleTrails();
                        status = std::string("universe trails ") + (universe.getTrailsEnabled() ? "enabled" : "disabled");
                    }
                }
                else if (mode == AppMode::HierarchySim)
                {
                    if (event.key.keysym.sym == SDLK_f || event.key.keysym.sym == SDLK_g ||
                        event.key.keysym.sym == SDLK_s || event.key.keysym.sym == SDLK_1 ||
                        event.key.keysym.sym == SDLK_2 || event.key.keysym.sym == SDLK_3 ||
                        event.key.keysym.sym == SDLK_b)
                    {
                        status = hierarchy.getStatus();
                    }
                    else if (event.key.keysym.sym == SDLK_z)
                    {
                        int mx = SIM_W / 2, my = SIM_H / 2;
                        SDL_GetMouseState(&mx, &my);
                        hierarchy.zoomIntoAt(mx, my);
                        status = hierarchy.getStatus();
                    }
                }
                else if (mode == AppMode::MazeArt)
                {
                    if (event.key.keysym.sym == SDLK_i)
                    {
                        std::string path = browseImageFilePath();
                        if (path.empty())
                        {
                            status = "browse canceled or no file dialog available";
                        }
                        else
                        {
                            bool ok = maze.loadImage(path);
                            status = ok ? ("image loaded: " + path) : ("failed load: " + path);
                        }
                    }
                    else if (event.key.keysym.sym == SDLK_u)
                    {
                        maze.unloadImage();
                        status = "image weights removed";
                    }
                    else if (event.key.keysym.sym == SDLK_k)
                    {
                        maze.toggleMonochrome();
                        status = std::string("maze monochrome ") + (maze.getMonochrome() ? "ON" : "OFF");
                    }
                }
            }
        }

        const Uint8 *keys = SDL_GetKeyboardState(nullptr);
        Uint32 now = SDL_GetTicks();
        if (now - lastThrottle > THROTTLE_MS)
        {
            bool any = false;
            if (keys[SDL_SCANCODE_UP])
            {
                if (mode == AppMode::ParticleLife)
                    sim.increaseSpeed();
                else if (mode == AppMode::GameOfLife)
                    gol.increaseTickRate();
                else if (mode == AppMode::LangtonsAnt)
                    ant.increaseSpeed();
                else if (mode == AppMode::Boids)
                    boids.increaseSpeed();
                else if (mode == AppMode::UniverseSim)
                    universe.increaseSpeed();
                else if (mode == AppMode::HierarchySim)
                    hierarchy.increaseTimeScale();
                else if (mode == AppMode::MazeArt)
                    maze.increaseSpeed();
                any = true;
            }
            if (keys[SDL_SCANCODE_DOWN])
            {
                if (mode == AppMode::ParticleLife)
                    sim.decreaseSpeed();
                else if (mode == AppMode::GameOfLife)
                    gol.decreaseTickRate();
                else if (mode == AppMode::LangtonsAnt)
                    ant.decreaseSpeed();
                else if (mode == AppMode::Boids)
                    boids.decreaseSpeed();
                else if (mode == AppMode::UniverseSim)
                    universe.decreaseSpeed();
                else if (mode == AppMode::HierarchySim)
                    hierarchy.decreaseTimeScale();
                else if (mode == AppMode::MazeArt)
                    maze.decreaseSpeed();
                any = true;
            }

            if (mode == AppMode::ParticleLife && keys[SDL_SCANCODE_EQUALS])
            {
                sim.addType();
                any = true;
            }
            if (mode == AppMode::ParticleLife && keys[SDL_SCANCODE_MINUS])
            {
                sim.removeType();
                any = true;
            }
            if (mode == AppMode::Boids && keys[SDL_SCANCODE_EQUALS])
            {
                boids.addFlock();
                any = true;
            }
            if (mode == AppMode::HierarchySim && keys[SDL_SCANCODE_EQUALS])
            {
                hierarchy.increasePopulation();
                status = hierarchy.getStatus();
                any = true;
            }
            if (mode == AppMode::Boids && keys[SDL_SCANCODE_MINUS])
            {
                boids.removeFlock();
                any = true;
            }
            if (mode == AppMode::HierarchySim && keys[SDL_SCANCODE_MINUS])
            {
                hierarchy.decreasePopulation();
                status = hierarchy.getStatus();
                any = true;
            }
            if (any)
                lastThrottle = now;
        }

        if (mode == AppMode::ParticleLife)
        {
            sim.update();
            sim.draw(simRen);
        }
        else if (mode == AppMode::GameOfLife)
        {
            gol.update(dt);
            gol.draw(simRen);
        }

        else if (mode == AppMode::LangtonsAnt)
        {
            ant.update(dt);
            ant.draw(simRen);
        }

        else if (mode == AppMode::ReactionDiffusion)
        {
            rd.update(dt);
            rd.draw(simRen);
        }

        else if (mode == AppMode::Boids)
        {
            boids.update(dt);
            boids.draw(simRen);
        }
        else if (mode == AppMode::AtomModel)
        {
            atom.update(dt);
            atom.draw(simRen);
        }
        else if (mode == AppMode::UniverseSim)
        {
            universe.update(dt);
            universe.draw(simRen);
        }
        else if (mode == AppMode::HierarchySim)
        {
            hierarchy.update(dt);
            hierarchy.draw(simRen);
            status = hierarchy.getStatus();
        }
        else if (mode == AppMode::MazeArt)
        {
            maze.update(dt);
            maze.draw(simRen);
        }

        PanelState panelState = {mode, fps, status};
        drawPanel(panRen, font, small, sim, gol, ant, rd, boids, atom, universe, hierarchy, maze, panelState);
    }

    TTF_CloseFont(font);
    TTF_CloseFont(small);
    TTF_Quit();
    IMG_Quit();

    SDL_DestroyRenderer(panRen);
    SDL_DestroyWindow(panWin);
    SDL_DestroyRenderer(simRen);
    SDL_DestroyWindow(simWin);
    SDL_Quit();
    return 0;
}