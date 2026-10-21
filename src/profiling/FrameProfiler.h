#pragma once
// Opt-in stage profiler for offline video exports. No GPU synchronization per frame.
// GPU timer queries are resolved only after all frames have been rendered.
#include <glad/gl.h>
#include <array>
#include <chrono>
#include <cstddef>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

enum class ProfileStage : std::size_t
{
    Simulation, TextureUpload, Scene, Meteors, Breakup, Particles, HDR, Readback, Output, Count
};

class FrameProfiler
{
public:
    static constexpr std::size_t StageCount = static_cast<std::size_t>(ProfileStage::Count);
    static constexpr std::array<const char*, StageCount> Names{{
        "simulation", "texture_upload", "scene", "meteors", "breakup",
        "particles", "hdr", "readback", "output"
    }};

    explicit FrameProfiler(bool enabled = false) : mEnabled(enabled) {}
    FrameProfiler(const FrameProfiler&) = delete;
    FrameProfiler& operator=(const FrameProfiler&) = delete;
    bool enabled() const { return mEnabled; }

    class CpuTimer
    {
    public:
        CpuTimer(FrameProfiler& profiler, ProfileStage stage)
            : mProfiler(profiler), mStage(stage),
              mStart(profiler.mEnabled ? Clock::now() : Clock::time_point{}) {}
        ~CpuTimer()
        {
            if (!mProfiler.mEnabled) return;
            const auto elapsed = std::chrono::duration<double, std::milli>(Clock::now() - mStart);
            const auto index = static_cast<std::size_t>(mStage);
            mProfiler.mCpuMs[index] += elapsed.count();
            ++mProfiler.mSamples[index];
        }
        CpuTimer(const CpuTimer&) = delete;
        CpuTimer& operator=(const CpuTimer&) = delete;

    private:
        using Clock = std::chrono::steady_clock;
        FrameProfiler& mProfiler;
        ProfileStage mStage;
        Clock::time_point mStart;
    };

    class GpuTimer
    {
    public:
        GpuTimer(FrameProfiler& profiler, ProfileStage stage) : mProfiler(profiler)
        {
            if (!profiler.mEnabled) return;
            glGenQueries(1, &mQuery);
            if (!mQuery) return;
            profiler.mGpuQueries[static_cast<std::size_t>(stage)].push_back(mQuery);
            glBeginQuery(GL_TIME_ELAPSED, mQuery);
        }
        ~GpuTimer() { if (mQuery) glEndQuery(GL_TIME_ELAPSED); }
        GpuTimer(const GpuTimer&) = delete;
        GpuTimer& operator=(const GpuTimer&) = delete;

    private:
        FrameProfiler& mProfiler;
        GLuint mQuery = 0;
    };

    void recordFrame() { if (mEnabled) ++mFrames; }
    void recordSimulationStep() { if (mEnabled) ++mSimulationSteps; }

    // Must be called while the same OpenGL context remains current.
    // Results are synchronization points at the end, never inside a frame.
    void writeJson(const std::string& path) const
    {
        if (!mEnabled) return;
        std::ofstream output(path);
        if (!output) throw std::runtime_error("Cannot open profiling report: " + path);
        output << "{\n  \"frames\": " << mFrames
               << ",\n  \"simulation_steps\": " << mSimulationSteps
               << ",\n  \"cpu_ms\": {\n";
        for (std::size_t i = 0; i < StageCount; ++i)
        {
            output << "    \"" << Names[i] << "\": " << mCpuMs[i]
                   << (i + 1 == StageCount ? "\n" : ",\n");
        }
        output << "  },\n  \"samples\": {\n";
        for (std::size_t i = 0; i < StageCount; ++i)
        {
            output << "    \"" << Names[i] << "\": " << mSamples[i]
                   << (i + 1 == StageCount ? "\n" : ",\n");
        }
        output << "  },\n  \"gpu_ms\": {\n";
        for (std::size_t i = 0; i < StageCount; ++i)
        {
            double totalMs = 0;
            for (GLuint query : mGpuQueries[i])
            {
                GLuint64 elapsed = 0;
                glGetQueryObjectui64v(query, GL_QUERY_RESULT, &elapsed);
                totalMs += double(elapsed) / 1000000.0;
            }
            output << "    \"" << Names[i] << "\": " << totalMs
                   << (i + 1 == StageCount ? "\n" : ",\n");
        }
        output << "  }\n}\n";
        if (!output) throw std::runtime_error("Cannot write profiling report: " + path);
    }

    void release()
    {
        for (auto& queries : mGpuQueries)
        {
            if (!queries.empty())
                glDeleteQueries(static_cast<GLsizei>(queries.size()), queries.data());
            queries.clear();
        }
    }

private:
    bool mEnabled = false;
    std::array<double, StageCount> mCpuMs{};
    std::array<std::size_t, StageCount> mSamples{};
    std::array<std::vector<GLuint>, StageCount> mGpuQueries{};
    std::size_t mFrames = 0, mSimulationSteps = 0;
};
