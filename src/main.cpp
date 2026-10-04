#include "Application.h"
#include "platform/ResourcePaths.h"
#include <iostream>
#include <string>

int main(int argc, char** argv)
{
    bool smokeTest = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--smoke-test") smokeTest = true;
        else {
            std::cerr << "Usage: project [--smoke-test]\n";
            return 2;
        }
    }
    try {
        // Resolve before GLFW, which can change the working directory on macOS.
        ResourcePaths::root();
        ApplicationOptions options;
        if (smokeTest) {
            options.visible = false;
            options.width = 640;
            options.height = 480;
        }
        Application app(options);
        if (!app.init()) return 1;
        if (smokeTest && !app.audioReady()) {
            std::cerr << "Smoke test failed: music or impact audio unavailable\n";
            return 1;
        }
        app.run(smokeTest ? 3 : 0);
        if (smokeTest) {
            if (glGetError() != GL_NO_ERROR) return 1;
            std::cout << "Scene, rendering, music and impact audio initialized successfully\n";
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
