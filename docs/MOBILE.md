# Android and iOS MVP

The mobile implementation shares the cinematic, simulation and SDL3 audio with
Windows/macOS/Linux. It uses SDL3 windows/events and OpenGL ES 3.0 instead of GLFW
and desktop OpenGL. Shader precision and scalar texture uploads are adapted for
ES; scene rendering is capped at 1280 pixels on its longest side. The control bar
stays at the native screen resolution. Half-float HDR uses the device's color
buffer extension; devices without it use an RGBA8 fallback.

## MVP controls

Launch in landscape. The bottom bar provides **Pause/Play**, **Restart**, **-10**,
**+10**, and **Mute**. The cinematic camera runs automatically. Desktop keyboard
controls remain available with a hardware keyboard. A complete touch-controlled
free-flight camera is outside this MVP.

Music and impact WAV files, shaders, models and textures are packaged with the
application. APK assets are read with SDL IO rather than native file streams.
Background events immediately pause the audio device; foreground events resume
according to the user's pause state and resynchronize the frame clock. On graphics
context loss the MVP exits with a diagnostic and can be relaunched.

## Build Android

Install Python 3, FFmpeg, JDK 17 and an Android SDK. Install these SDK components:

```bash
sdkmanager "platforms;android-35" "ndk;28.2.13676358" "cmake;3.22.1"
python3 scripts/prepare_mobile.py
cd mobile/android
./gradlew assembleDebug
```

On Windows use `gradlew.bat`. Set `ANDROID_HOME` to the SDK directory or provide
`mobile/android/local.properties` with `sdk.dir=...`.

The APK is `mobile/android/app/build/outputs/apk/debug/app-debug.apk`; install it
with `adb install -r <apk>`. It includes ARM64 for phones and x86-64 for emulators.
Minimum Android version is 8.0 (API 26), with OpenGL ES 3.0. Debug APKs are signed
for development and can be installed directly; store distribution requires a
release signing key and store-specific preparation.

## Build iOS

Use a Mac with Xcode (including an iOS simulator), CMake, Python 3 and FFmpeg.
From the repository root:

```bash
python3 scripts/prepare_mobile.py
cmake -S . -B build/ios-simulator -G Xcode \
  -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT=iphonesimulator \
  -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=13.0 \
  -DFETCHCONTENT_SOURCE_DIR_SDL3="$PWD/build/mobile-deps/SDL" \
  -DFETCHCONTENT_SOURCE_DIR_GLM="$PWD/build/mobile-deps/glm" \
  -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED=NO -DBUILD_TESTING=OFF
cmake --build build/ios-simulator --config Release --target project --parallel 3
```

For an Intel Mac change the simulator architecture to `x86_64`. The application
bundle is `build/ios-simulator/runtime/project.app`.

For an iPhone use a separate build directory with `CMAKE_OSX_SYSROOT=iphoneos`
and architecture `arm64`. Configure signing in the generated Xcode project with
your development team and an available bundle identifier. An unsigned device
bundle from CI cannot be installed directly on an iPhone. Minimum deployment
version is iOS 13. The MVP uses OpenGL ES; a future Metal renderer is a separate
project rather than a prerequisite to this prototype.

## Automated verification

The Mobile MVP GitHub workflow builds an Android APK and runs it in an Android
emulator. It builds an iOS simulator app, runs it in an iPhone simulator and also
builds an unsigned ARM64 iPhone app. Logs and screenshots accompany the artifacts.

`--mobile-test` checks the actual scene, the advancing audio clock, pause/play,
restart, seek and mute via injected SDL touch events, then renders cinematic
stages at 40, 75 and 105 seconds. It excludes the control bar when checking that
the rendered image is nonuniform. `--keep-running` keeps the tested app open for
screenshots and OS background/foreground checks. These tests exercise the audio
playback path; physical listening and device performance still need real devices.

On a Linux development machine the same GLES backend can be checked with:

```bash
cmake -S . -B build/mobile-gles -DPROJECT_MOBILE=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build/mobile-gles --parallel 2
SDL_AUDIO_DRIVER=dummy build/mobile-gles/runtime/project --mobile-test
```

Source references: [SDL Android](https://wiki.libsdl.org/SDL3/README-android),
[SDL iOS](https://wiki.libsdl.org/SDL3/README-ios).
