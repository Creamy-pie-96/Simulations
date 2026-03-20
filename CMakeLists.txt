cmake_minimum_required(VERSION 3.16)
project(ParticleLife)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 REQUIRED)

# SDL2_ttf
find_path(SDL2_TTF_INCLUDE_DIR SDL_ttf.h PATH_SUFFIXES SDL2)
find_library(SDL2_TTF_LIBRARY NAMES SDL2_ttf)

# SDL2_image
find_path(SDL2_IMAGE_INCLUDE_DIR SDL_image.h PATH_SUFFIXES SDL2)
find_library(SDL2_IMAGE_LIBRARY NAMES SDL2_image)

add_executable(particle_life
    src/main.cpp
    src/Camera.cpp
    src/LevelManager.cpp
    src/MazeArt.cpp
    src/Particle.cpp
    src/AtomModel.cpp
    src/GameOfLife.cpp
    src/LangtonsAnt.cpp
    src/ReactionDiffusion.cpp
    src/Boids.cpp
    src/levels/GalaxyMapLevel.cpp
    src/levels/GalaxyLevel.cpp
    src/levels/SolarSystemLevel.cpp
    src/levels/PlanetLevel.cpp
    src/levels/BlackHoleLevel.cpp
    src/UniverseSim.cpp
    src/Simulation.cpp
)

target_include_directories(particle_life PRIVATE
    include
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_TTF_INCLUDE_DIR}
    ${SDL2_IMAGE_INCLUDE_DIR}
)

target_link_libraries(particle_life PRIVATE
    ${SDL2_LIBRARIES}
    ${SDL2_TTF_LIBRARY}
    ${SDL2_IMAGE_LIBRARY}
)

set(CMAKE_BUILD_TYPE Release)
target_compile_options(particle_life PRIVATE -O2)