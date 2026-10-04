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

- Linux with an X11 display, or macOS (CMake build)
- C++17 compiler and CMake 3.20+ (or GNU Make)
- OpenGL 3.3+
- GLFW 3, GLEW, GLM and SDL3 development packages
- FFmpeg

The project expects the libraries to be discoverable through `pkg-config`:
`glfw3`, `glew` and `sdl3`.

## Build and run

### CMake (recommended)

Install OpenGL, GLFW, GLEW, GLM, SDL3 and FFmpeg first. On Fedora:

```bash
sudo dnf install gcc-c++ cmake make glfw-devel glew-devel glm-devel SDL3-devel ffmpeg
```

On macOS:

```bash
brew install cmake glfw glew glm sdl3 ffmpeg
```

Ubuntu 24.04 requires SDL3 to be built from source; the Dockerfile and GitHub
workflow handle this automatically.

Run from the project directory:

```bash
cmake -S . -B build/cmake -DCMAKE_BUILD_TYPE=Release
cmake --build build/cmake --parallel 2
ctest --test-dir build/cmake --output-on-failure
cmake --build build/cmake --target run
```

The executable and assets are staged under `build/cmake/runtime/`, including
converted audio under `build/cmake/runtime/build/music/`. You can also run
`cd build/cmake/runtime && ./project`. To enable the graphical integration test:

```bash
cmake -S . -B build/cmake -DENABLE_RUNTIME_TESTS=ON
cmake --build build/cmake --parallel 2
ctest --test-dir build/cmake -L runtime --output-on-failure
```

On a Linux server with `xvfb` and `xauth` installed, use:

```bash
LIBGL_ALWAYS_SOFTWARE=1 xvfb-run -a ctest --test-dir build/cmake -L runtime --output-on-failure
```

Assertions remain enabled in the test executables even for Release builds.
To install the executable and assets into another directory, run
`cmake --install build/cmake --prefix /path/to/installation` after building,
then launch `./project` from that installation directory. Shared libraries
must also be installed on the destination machine.

### GNU Make

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

## Docker

Build a Linux image from the project directory:

```bash
docker build -t project-opengl .
```

The multi-stage build compiles SDL3 3.2.10 and the application, runs all CPU,
audio and OpenGL tests under Xvfb, then keeps only the application, assets and
runtime libraries in the final image.

To open the application on a Linux X11 desktop (including XWayland), pass the
display socket and your X11 authorization file:

```bash
docker run --rm \
  --user "$(id -u):$(id -g)" \
  -e DISPLAY \
  -e XAUTHORITY=/tmp/container.xauth \
  -e LIBGL_ALWAYS_SOFTWARE=1 \
  -e SDL_AUDIO_DRIVER=dummy \
  -v /tmp/.X11-unix:/tmp/.X11-unix:ro \
  -v "${XAUTHORITY:-$HOME/.Xauthority}:/tmp/container.xauth:ro" \
  project-opengl
```

The Xauthority file must exist and authorize the current display. This command
uses software rendering and silent audio; desktop audio/GPU forwarding depends
on the host configuration. The native CMake build provides normal desktop audio.
The container desktop launch is intended for Linux hosts.

## GitHub Actions (CI/CD)

The workflow is `.github/workflows/cmake-multi-platform.yml` (the directory name
is **workflows**, plural). It runs on pushes, pull requests and manual triggers:

- CMake builds in Debug and Release on Linux and macOS using Clang, plus GCC
  Release on Linux.
- The 13 CPU/audio tests run on both systems; the OpenGL test runs on Linux
  using Xvfb and Mesa software rendering.
- After every build job succeeds, Docker builds the image and repeats the tests
  in the Ubuntu container.
- A push to the repository's default branch also publishes
  `ghcr.io/hicha-m/project-opengl:latest` and a `sha-<commit>` tag. Other pushes
  and pull requests build without publishing.

Commit and push these files to GitHub to activate the workflow. Publication uses
GitHub's built-in `GITHUB_TOKEN` with `packages: write`; no Docker Hub account or
personal registry token is required. If organization settings restrict that
permission, allow package publishing for the workflow in the repository settings.
Check the **Actions** tab for builds and the repository's **Packages** area for
the image. This delivery step publishes a container image; it does not deploy a
running application to a server.

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
