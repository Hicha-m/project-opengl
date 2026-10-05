#include "Application.h"
#include "platform/ResourcePaths.h"
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

namespace
{
    struct CommandLineOptions
    {
        bool smokeTest = false;
        bool softwareContext = false;
        bool exportFrames = false;
        int width = 1280, height = 720, fps = 30;
        double duration = 110;
        std::string impactFile;
    };

    CommandLineOptions parseArguments(int argc, char** argv)
    {
        CommandLineOptions options;
        for (int i = 1; i < argc; ++i)
        {
            const std::string arg(argv[i]);
            auto value = [&]() -> std::string
            {
                if (++i >= argc)
                {
                    throw std::runtime_error("Missing value for " + arg);
                }
                return argv[i];
            };
            if (arg == "--smoke-test")
            {
                options.smokeTest = true;
            }
            else if (arg == "--software-context")
            {
                options.softwareContext = true;
            }
            else if (arg == "--export-frames")
            {
                options.exportFrames = true;
            }
            else if (arg == "--width")
            {
                options.width = std::stoi(value());
            }
            else if (arg == "--height")
            {
                options.height = std::stoi(value());
            }
            else if (arg == "--fps")
            {
                options.fps = std::stoi(value());
            }
            else if (arg == "--duration")
            {
                options.duration = std::stod(value());
            }
            else if (arg == "--impact-log")
            {
                options.impactFile = value();
            }
            else
            {
                throw std::runtime_error("Unknown argument: " + arg);
            }
        }
        if ((options.softwareContext && !options.smokeTest && !options.exportFrames) ||
            (options.smokeTest && options.exportFrames) || options.width <= 0 || options.height <= 0 ||
            options.width > 7680 || options.height > 4320 || options.fps <= 0 || options.fps > 240 ||
            !std::isfinite(options.duration) || options.duration <= 0 || options.duration > 3600)
        {
            throw std::runtime_error("Invalid command-line settings");
        }
        return options;
    }

    void exportVideo(Application& app, const CommandLineOptions& options)
    {
        std::ofstream impacts(std::filesystem::u8path(options.impactFile));
        if (!impacts)
        {
            throw std::runtime_error("Cannot open impact log");
        }
        app.renderVideo(
            options.width, options.height, options.fps, options.duration,
            [](const unsigned char* pixels, std::size_t size)
            {
                if (std::fwrite(pixels, 1, size, stdout) != size)
                {
                    throw std::runtime_error("Cannot write video frame");
                }
            },
            [&](float time, float gain) { impacts << time << ' ' << gain << '\n'; });
        if (std::fflush(stdout) != 0 || !impacts)
        {
            throw std::runtime_error("Cannot finish video export");
        }
    }

    bool verifyRenderedFrame(const ApplicationOptions& options)
    {
        std::vector<unsigned char> pixels(options.width * options.height * 4);
        glReadPixels(0, 0, options.width, options.height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        if (glGetError() != GL_NO_ERROR)
        {
            return false;
        }
        bool varied = false;
        for (std::size_t i = 4; i < pixels.size(); i += 4)
        {
            if (pixels[i] != pixels[0] || pixels[i + 1] != pixels[1] || pixels[i + 2] != pixels[2])
            {
                varied = true;
            }
        }
        if (!varied)
        {
            std::cerr << "Smoke test failed: rendered image is blank\n";
            return false;
        }
        std::cout << "Scene, rendering, music and impact audio initialized successfully\n";
        return true;
    }
} // namespace

int main(int argc, char** argv)
{
    try
    {
        const auto command = parseArguments(argc, argv);
        // stdout is exclusively a binary RGB stream during export.
        if (command.exportFrames)
        {
            std::cout.rdbuf(std::cerr.rdbuf());
#ifdef _WIN32
            _setmode(_fileno(stdout), _O_BINARY);
#endif
        }
        // Resolve before GLFW, which can change the working directory on macOS.
        ResourcePaths::root();
        ApplicationOptions options;
        options.softwareContext = command.softwareContext;
        options.width = command.smokeTest ? 640 : command.width;
        options.height = command.smokeTest ? 480 : command.height;
        options.visible = !command.smokeTest && !command.exportFrames;
        options.music = !command.exportFrames;
        Application app(options);
        if (!app.init())
        {
            return 1;
        }
        if (command.smokeTest && !app.audioReady())
        {
            std::cerr << "Smoke test failed: music or impact audio unavailable\n";
            return 1;
        }
        if (command.exportFrames)
        {
            exportVideo(app, command);
        }
        else
        {
            app.run(command.smokeTest ? 3 : 0);
        }
        return command.smokeTest && !verifyRenderedFrame(options) ? 1 : 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
