#include "Application.h"
#ifdef PROJECT_MOBILE
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>
#include <cmath>
#include <stdexcept>
#endif
#include "platform/ResourcePaths.h"
#include "platform/AndroidLog.h"
#include <iostream>
#include <string>
#include <vector>

namespace {
bool imageAvailable() {
    GLint viewport[4]; glGetIntegerv(GL_VIEWPORT, viewport);
    const int bottom = viewport[3] / 3; // Exclude controls and mobile safe-area margins.
    const int height = viewport[3] - bottom;
    if (viewport[2] <= 0 || height <= 0) return false;
    std::vector<unsigned char> pixels(std::size_t(viewport[2]) * height * 4);
    glReadPixels(0, bottom, viewport[2], height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    if (const auto error = glGetError(); error != GL_NO_ERROR) {
        std::cerr << "OpenGL error: " << error << '\n'; return false;
    }
    for (std::size_t i = 4; i < pixels.size(); i += 4)
        if (pixels[i] != pixels[0] || pixels[i + 1] != pixels[1] || pixels[i + 2] != pixels[2]) return true;
    return false;
}
#ifdef PROJECT_MOBILE
void mobileTest(Application& app) {
    auto require = [](bool condition, const char* message) { if (!condition) throw std::runtime_error(message); };
    require(app.audioReady(), "Mobile test: audio unavailable");
    app.run(3, false);
    require(imageAvailable(), "Mobile test: blank scene or GL error");
    auto tap = [&](int button) {
        SDL_Event event{}; event.type = SDL_EVENT_FINGER_DOWN;
        auto* window = SDL_GL_GetCurrentWindow();
        int width, height; SDL_GetWindowSize(window, &width, &height);
        SDL_Rect safe{};
        if (!SDL_GetWindowSafeArea(window, &safe)) { safe.w = width; safe.h = height; }
        event.tfinger.x = (safe.x + safe.w * (button + 0.5f) / 5.0f) / width;
        event.tfinger.y = (safe.y + safe.h * 0.93f) / height;
        require(SDL_PushEvent(&event), "Cannot inject touch event"); app.run(1, false);
    };
    tap(0); require(app.sequencePaused(), "Pause touch failed");
    const float paused = app.sequenceTime(); SDL_Delay(100); app.run(2, false);
    require(std::abs(app.sequenceTime()-paused) < 0.001f, "Paused timeline advanced");
    tap(3); require(std::abs(app.sequenceTime()-paused-10) < 0.02f, "Seek forward touch failed");
    tap(2); require(std::abs(app.sequenceTime()-paused) < 0.02f, "Seek backward touch failed");
    tap(4); require(app.audioMuted(), "Mute touch failed");
    tap(4); require(!app.audioMuted(), "Unmute touch failed");
    tap(0); require(!app.sequencePaused(), "Resume touch failed");
    SDL_Delay(150); app.run(2, false);
    require(app.sequenceTime() > paused + 0.02f, "Playback clock did not resume");
    tap(1); require(!app.sequencePaused() && app.sequenceTime() < 1.0f, "Restart touch failed");
    app.toggleSequencePause();
    for (float seconds : {40.0f, 75.0f, 105.0f}) {
        app.seekSequence(seconds); app.run(1, false);
        require(imageAvailable(), "Cinematic stage failed to render");
    }
    app.restartSequence(); app.run(1);
    std::cout << "MOBILE_MVP_TEST_PASSED: scene, cinematic stages, audio clock and touch controls" << std::endl;
}
#endif
}

int main(int argc, char** argv)
{
#ifdef __ANDROID__
    AndroidLogBuffer output(std::cout), errors(std::cerr);
#endif
#ifdef PROJECT_MOBILE
    std::cout << std::unitbuf;
#endif
    bool smokeTest = false;
    bool mobileTesting = false, keepRunning = false;
    bool softwareContext = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--smoke-test") smokeTest = true;
        else if (std::string(argv[i]) == "--mobile-test") mobileTesting = true;
        else if (std::string(argv[i]) == "--keep-running") keepRunning = true;
        else if (std::string(argv[i]) == "--software-context") softwareContext = true;
        else {
            std::cerr << "Usage: project [--smoke-test [--software-context]]\n";
            return 2;
        }
    }
    if (softwareContext && !smokeTest) return 2;
    try {
        // Resolve before GLFW, which can change the working directory on macOS.
        ResourcePaths::root();
        ApplicationOptions options;
        options.softwareContext = softwareContext;
        if (smokeTest) {
            options.visible = false;
            options.width = 640;
            options.height = 480;
        }
        Application app(options);
        if (!app.init()) {
            if (mobileTesting) std::cerr << "MOBILE_MVP_TEST_FAILED: startup failed\n";
            return 1;
        }
        if (smokeTest && !app.audioReady()) {
            std::cerr << "Smoke test failed: music or impact audio unavailable\n";
            return 1;
        }
#ifdef PROJECT_MOBILE
        if (mobileTesting) {
            mobileTest(app);
            if (keepRunning) app.run();
            return 0;
        }
#else
        if (mobileTesting) return 2;
#endif
        app.run(smokeTest ? 3 : 0, !smokeTest);
        if (smokeTest) {
            if (!imageAvailable()) {
                std::cerr << "Smoke test failed: blank image or rendering error\n";
                return 1;
            }
            std::cout << "Scene, rendering, music and impact audio initialized successfully\n";
        }
        return 0;
    } catch (const std::exception& error) {
        if (mobileTesting) std::cerr << "MOBILE_MVP_TEST_FAILED: ";
        std::cerr << error.what() << '\n';
        return 1;
    }
}
