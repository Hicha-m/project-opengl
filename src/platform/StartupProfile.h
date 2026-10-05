#pragma once
#include <array>
#include <chrono>
#include <iostream>

namespace StartupProfile
{
    using Clock = std::chrono::steady_clock;
    enum Stage
    {
        Shuttle,
        Shaders,
        Textures
    };
    inline bool enabled = false;
    inline bool reported = false;
    inline Clock::time_point start;
    inline std::array<double, 3> milliseconds{};
    inline unsigned shaderCompiles = 0, textureHits = 0;
    inline void begin(bool diagnostic)
    {
        enabled = diagnostic;
        reported = false;
        milliseconds = {};
        shaderCompiles = textureHits = 0;
        start = Clock::now();
    }
    struct Timer
    {
        Stage stage;
        Clock::time_point start = Clock::now();
        explicit Timer(Stage s) : stage(s) {}
        ~Timer()
        {
            if (enabled)
            {
                milliseconds[stage] +=
                    std::chrono::duration<double, std::milli>(Clock::now() - start).count();
            }
        }
    };
    inline void firstFrame()
    {
        if (!enabled || reported)
        {
            return;
        }
        reported = true;
        std::cerr << "Startup: shuttle " << milliseconds[Shuttle] << " ms, shaders " << milliseconds[Shaders]
                  << " ms (" << shaderCompiles << " compiled), textures " << milliseconds[Textures] << " ms ("
                  << textureHits << " shared), first frame "
                  << std::chrono::duration<double, std::milli>(Clock::now() - start).count() << " ms\n";
    }
} // namespace StartupProfile
