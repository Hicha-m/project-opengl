# Mobile assessment

Windows, macOS and Linux are the priority. This repository currently uses
GLFW windows and keyboard/mouse input with GLEW and desktop GLSL 330 shaders.
GLFW has no Android/iOS window backend. Neither changing the compiler nor
using Docker makes this desktop executable a mobile app.

## Reusable parts

The C++17 timeline, camera choreography, meteor simulation, collision and particle
systems can be reused. SDL3 audio also supports Android and iOS. The resource
path layer isolates desktop file locations so a mobile implementation can replace
resource access without changing scene asset names.

## Work needed for a usable mobile version

1. Replace GLFW with an SDL3 window/event layer and a lifecycle that handles
   suspend, resume, audio interruption and graphics-context recreation.
2. Replace GLEW with an OpenGL ES loader; port GLSL 330 shaders to GLSL ES 300
   and validate HDR float attachments, blending and framebuffer formats.
   On iOS, assess a Metal backend rather than relying long-term on deprecated GL.
3. Package Android resources into APK assets and read them through SDL IO;
   `std::ifstream` cannot directly read compressed APK assets. Package iOS
   resources in the application bundle.
4. Add touch UI for pause/restart/seek, camera interaction and mute, and scale
   the rendering workload for mobile GPUs and screen dimensions.
5. Build and test an Android APK on an emulator and actual GPU. Build an iOS
   simulator app and test a device build with the user's Apple signing profile.

A mobile release is not claimed until startup, complete animation, audio,
controls and suspend/resume are verified on those targets.

References: [GLFW platforms](https://www.glfw.org/),
[SDL3 Android](https://wiki.libsdl.org/SDL3/README-android),
[SDL3 iOS](https://wiki.libsdl.org/SDL3/README-ios).
