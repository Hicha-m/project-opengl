# OpenGL Space Cinematic

Real-time OpenGL project featuring a cinematic journey from an Earth under
meteor bombardment to a wide shot of the Solar System and the Milky Way.

![Space cinematic preview](image.gif)

## Overview

This project combines a scripted camera sequence with real-time rendering and
simulation. It demonstrates a complete OpenGL scene rather than a collection
of isolated effects:

- Earth lighting with day/night, normal, specular, damage and heat maps.
- Deterministic meteor showers, collisions, trails and impact effects.
- Progressive crust damage, cracks and a 32-piece Earth breakup.
- HDR rendering, bloom, emissive materials and impact lights.
- A shuttle flight followed by a Solar System and Milky Way reveal.
- Music and impact sounds synchronized with the cinematic timeline.
- A free FPS camera for exploring the scene while the sequence continues.

## Features

### Cinematic sequence

The main sequence lasts approximately 110 seconds. It starts with an intact
Earth, builds through several meteor waves, triggers the destruction sequence,
then pulls back from the shuttle to the Solar System and the galaxy.

### Meteor simulation

Meteors are generated on the CPU with reproducible random seeds. Their
continuous collision detection prevents tunneling at high speed. Impacts feed
the damage, heat, particle, light and audio systems independently.

### Earth destruction

Impact energy accumulates into a global destruction level. The surface develops
burn marks, temporary heat and procedural cracks before the Earth separates into
animated fragments around an emissive core.

### Rendering

The renderer uses shared meshes and materials, instanced particles, textured
planets, transparent Saturn rings and an HDR pipeline with bloom. The design
keeps simulation, scene management and rendering in separate modules.

### Audio

The cinematic music drives the visible timeline when it is available. FFmpeg
converts the supplied MP3 files to WAV during the build, while SDL3 handles
playback, volume, mute and impact sounds.

## Requirements

- Linux with an X11 display
- C++17 compiler and GNU Make
- OpenGL 3.3+
- GLFW 3, GLEW, GLM and SDL3 development packages
- FFmpeg

The project expects the libraries to be discoverable through `pkg-config`:
`glfw3`, `glew` and `sdl3`.

## Build and run

Run these commands from the project directory:

```bash
make              # Build the application
make run          # Build and launch the cinematic
make test         # Run CPU and audio tests
make test-runtime # Run the hidden-window OpenGL test suite
make clean        # Remove build files and generated executables
```

`make test-runtime` needs a working X11 display even though the test window is
hidden. The build generates converted audio files in `build/music/`; the
original MP3 files remain unchanged.

## Controls

| Key | Action |
| --- | --- |
| `Space` | Pause or resume the sequence and music |
| `R` | Restart the complete cinematic |
| `Left` / `Right` | Seek backward or forward by 10 seconds |
| `Up` / `+` | Double playback speed, up to 8x |
| `Down` / `-` | Halve playback speed, down to 0.25x |
| `0` | Restore normal speed |
| `M` | Mute or unmute audio |
| `F3` | Switch between cinematic and FPS camera |
| `W` / `S` | Move forward or backward in FPS mode |
| `A` / `D` | Strafe left or right in FPS mode |
| `Z` / `X` | Move up or down in FPS mode |
| Mouse | Look around in FPS mode |
| `G` / `H` | Increase or decrease FPS movement speed |
| `F1` | Toggle wireframe rendering |
| `F2` | Toggle camera information |
| `Esc` | Quit |

## Project structure

```text
src/
├── Application.cpp       Application loop, input and system orchestration
├── animation/            Timeline, keyframes and animation tracks
├── audio/                SDL3 music and impact playback
├── camera/               FPS, orbit and cinematic cameras
├── cinematic/            Main film sequence and scene choreography
├── geometry/             Sphere generation and collision geometry
├── graphics/             Meshes, shaders, textures, HDR and particles
├── scene/                Objects, materials, lights and scene setup
└── systems/              Meteors, solar system, damage, breakup and particles

shaders/                  GLSL vertex and fragment shaders
textures/                 Planet, space and effect textures
models/                   Shuttle model and material data
audio/                    Music and impact sound sources
tests/                    CPU, audio and OpenGL integration tests
```

## Testing

The test suite covers deterministic replay, animation, orbital motion, meteor
spawning and collision, particle and trail emission, impact lights, Earth
damage and heat, destruction level, breakup state and audio behavior.

The runtime tests also exercise OpenGL resource loading, HDR/bloom rendering,
occlusion, particle instancing, reset behavior and full-sequence replay.
Generated diagnostic captures are written to `/tmp` by the runtime tests.

## Assets

The project uses the supplied textures, shuttle model and audio files. Paths
are resolved relative to the project root, so launch the application with
`make run` or from this directory:

```bash
./project
```

## License

No license has been specified for this project yet.
