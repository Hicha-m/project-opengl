#include "Application.h"
#include "platform/ResourcePaths.h"
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <cstdio>
#include <cmath>
#include <stdexcept>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

int main(int argc, char** argv)
{
    bool smokeTest = false;
    bool softwareContext = false;
    bool exportFrames = false;
    int width = 1280, height = 720, fps = 30;
    double duration = 110;
    std::string impactFile;
    try {
        for (int i = 1; i < argc; ++i) {
            const std::string arg(argv[i]);
            auto value = [&]() -> std::string {
                if (++i >= argc) throw std::runtime_error("Missing value for " + arg);
                return argv[i];
            };
            if (arg == "--smoke-test") smokeTest = true;
            else if (arg == "--software-context") softwareContext = true;
            else if (arg == "--export-frames") exportFrames = true;
            else if (arg == "--width") width = std::stoi(value());
            else if (arg == "--height") height = std::stoi(value());
            else if (arg == "--fps") fps = std::stoi(value());
            else if (arg == "--duration") duration = std::stod(value());
            else if (arg == "--impact-log") impactFile = value();
            else throw std::runtime_error("Unknown argument: " + arg);
        }
        if ((softwareContext && !smokeTest && !exportFrames) || (smokeTest && exportFrames)
            || width <= 0 || height <= 0 || width > 7680 || height > 4320 || fps <= 0 || fps > 240
            || !std::isfinite(duration) || duration <= 0 || duration > 3600)
            throw std::runtime_error("Invalid command-line settings");
        // stdout is exclusively a binary RGB stream during export.
        if (exportFrames) {
            std::cout.rdbuf(std::cerr.rdbuf());
#ifdef _WIN32
            _setmode(_fileno(stdout), _O_BINARY);
#endif
        }
        // Resolve before GLFW, which can change the working directory on macOS.
        ResourcePaths::root();
        ApplicationOptions options;
        options.softwareContext = softwareContext;
        options.width = width;
        options.height = height;
        if (exportFrames) { options.visible = false; options.music = false; }
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
        if (exportFrames) {
            std::ofstream impacts(std::filesystem::u8path(impactFile));
            if (!impacts) throw std::runtime_error("Cannot open impact log");
            app.renderVideo(width, height, fps, duration,
                [](const unsigned char* pixels, std::size_t size) {
                    if (std::fwrite(pixels, 1, size, stdout) != size)
                        throw std::runtime_error("Cannot write video frame");
                },
                [&](float time, float gain) { impacts << time << ' ' << gain << '\n'; });
            if (std::fflush(stdout) != 0 || !impacts)
                throw std::runtime_error("Cannot finish video export");
        } else app.run(smokeTest ? 3 : 0);
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
