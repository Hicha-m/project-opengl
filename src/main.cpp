#include "Application.h"
#include "platform/ResourcePaths.h"
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char** argv)
{
    bool smokeTest = false;
    bool softwareContext = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--smoke-test") smokeTest = true;
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
        if (!app.init()) return 1;
        if (smokeTest && !app.audioReady()) {
            std::cerr << "Smoke test failed: music or impact audio unavailable\n";
            return 1;
        }
        app.run(smokeTest ? 3 : 0);
        if (smokeTest) {
            std::vector<unsigned char> pixels(options.width * options.height * 4);
            glReadPixels(0, 0, options.width, options.height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
            if (glGetError() != GL_NO_ERROR) return 1;
            bool varied = false;
            for (std::size_t i = 4; i < pixels.size(); i += 4)
                if (pixels[i] != pixels[0] || pixels[i + 1] != pixels[1] || pixels[i + 2] != pixels[2]) varied = true;
            if (!varied) {
                std::cerr << "Smoke test failed: rendered image is blank\n";
                return 1;
            }
            std::cout << "Scene, rendering, music and impact audio initialized successfully\n";
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
