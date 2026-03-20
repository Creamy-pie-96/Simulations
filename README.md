# Particle Life + Physics Sandbox (SDL2)

Multi-mode interactive simulation app using C++17 + SDL2, SDL2_ttf, and SDL2_image.

It includes emergent systems, cellular automata, flocking, atom visualization, Newtonian gravity sandboxes, a hierarchical zoom universe, and image-guided maze art.

## Requirements

- C++17 compiler
- SDL2
- SDL2_ttf
- SDL2_image
- CMake

Ubuntu / Debian:

```bash
sudo apt install build-essential cmake libsdl2-dev libsdl2-ttf-dev libsdl2-image-dev
```

## Build

```bash
cmake -S . -B build
cmake --build build -j
```

## Run

```bash
cd build
./particle_life
```

## Modes (press **M** to cycle)

1. Particle Life
2. Game of Life
3. Langton's Ant
4. Reaction Diffusion
5. Boids
6. Atom Model
7. Universe Sim
8. Hierarchy Sim
9. Maze Art

## Global controls

- **M**: next mode
- **ESC**: quit
- **R**: randomize current mode (if supported)
- **C**: clear/reset current mode (if supported)
- **UP / DOWN**: speed/tick control (mode-specific)

## Mode details and controls

### 1) Particle Life

- Pairwise attraction/repulsion particle system with rule matrix
- Dynamic HSV color groups
- Trails mode (motion blur)
- Save/load rule preset

Controls:

- **LMB drag**: attract to cursor
- **RMB drag**: repel from cursor
- **T**: toggle trails
- **P**: save rule preset
- **L**: load rule preset
- **= / -**: add/remove color type
- **UP / DOWN**: speed

### 2) Game of Life

- Classic Conway rules
- Cell aging + fading visual states
- Pattern stamping

Controls:

- **LMB drag**: draw alive cells
- **RMB drag**: erase
- **1 / 2 / 3**: glider / pulsar / Gosper gun at cursor
- **UP / DOWN**: tick rate

### 3) Langton's Ant

- Multi-ant rule system
- Dynamic ant colors

Controls:

- **A**: add ant
- **LMB click**: add ant at cursor + paint black
- **LMB drag**: paint black
- **RMB drag**: paint white
- **UP / DOWN**: tick rate

### 4) Reaction Diffusion (Gray-Scott)

- Two-field chemical model (A/B)
- Multiple parameter presets
- Interactive brush editing

Controls:

- **N**: next preset
- **LMB drag**: inject chemical
- **RMB drag**: erase chemical
- **R**: reseed
- **C**: clear

### 5) Boids

- Separation / alignment / cohesion flocking
- Multiple dynamic flock color groups

Controls:

- **LMB drag**: attract boids
- **RMB drag**: repel boids
- **= / -**: add/remove flock
- **UP / DOWN**: max speed

### 6) Atom Model

- Quantized shell occupancy for element electron counts
- Supports periodic-table atomic numbers (1..118)
- Orbital probability cloud modes (`s`, `p`, `d`) with Langevin-like cloud motion
- Photon emission visual effect with Balmer-like colors
- Optional trails for cloud history

Controls:

- **LEFT / RIGHT**: previous/next element
- **O**: cycle orbital cloud (`s` / `p` / `d`)
- **LMB drag**: attract atom center
- **RMB drag**: repel atom center
- **T**: toggle trails
- **R**: random element
- **C**: reset to Hydrogen

### 7) Universe Sim

- Newtonian $N$-body style galaxy sandbox with gravitational softening
- Galaxy starts with stars + particles (no forced central black hole)
- Click-spawn black holes and stars
- Body merger detection (collisions combine mass/radius)
- Mouse gravitational interaction
- Optional trails for orbital paths

Controls:

- **LMB click**: spawn black hole
- **RMB click**: spawn star
- **MMB click**: remove nearest body
- **LMB drag**: attract bodies
- **RMB drag**: repel bodies
- **UP / DOWN**: simulation time scale
- **T**: toggle trails
- **R**: reseed galaxy
- **C**: clear

### 8) Hierarchy Sim

- Stack-based zoom levels:
  - Galaxy Map → Galaxy → Solar System → Planet / Black Hole
- Camera pan/zoom/focus workflow
- Per-level spawn and gravity interactions
- Per-level population and time scaling

Controls:

- **F** or **1**: focus mode
- **G** or **2**: gravity mode
- **S** or **3**: spawn mode
- **B**: zoom out one level
- **Z**: zoom into body at cursor
- **LMB / RMB** in current click mode
- **MMB drag**: pan
- **Mouse wheel**: zoom
- **= / -**: increase/decrease population
- **UP / DOWN**: increase/decrease time scale
- **R** or **C**: reset hierarchy root

### 9) Maze Art

- DFS recursive-backtracker maze generator
- Optional image-weighted traversal (darker pixels visited sooner)
- Corridor rendering is line-based (drawn paths), not tile-reveal
- Color mode from image or random palette
- Monochrome mode for pure brightness-based drawing
- Active carving head is green; idle marker is white

Controls:

- **UP / DOWN**: speed (supports fractional rates down to 0.1 steps/frame)
- **R**: regenerate maze
- **I**: browse/load image (`png`, `jpg`, `jpeg`, `bmp`)
- **U**: unload image weights (random perfect maze behavior)
- **K**: toggle monochrome

Notes:

- File picker uses one of: `zenity`, `kdialog`, `yad`, `qarma`.
- If no picker is available, image browsing will be canceled gracefully.

## Preset path (Particle Life)

Rule preset uses a stable writable SDL path:

- `SDL_GetPrefPath("ParticleLife", "ParticleLife") + "particle_rules.preset"`

The absolute path is printed to terminal at startup and on save/load.

## Performance notes

- Reaction Diffusion uses `cellSize = 3` by default for good performance.
- Universe Sim is $O(N^2)$ in body count, so very large body counts will reduce FPS.
- Hierarchy levels and Maze Art speed settings can also significantly affect frame time.

## Font note

The app loads:

- `/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf`
- `/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf`

Install DejaVu fonts if text does not appear.
