#pragma once
#include "systems/Meteor.h"
#include "systems/ParticleSystem.h"
#include <random>
#include <unordered_map>

struct MeteorTrailConfig
{
    float spacing = 0.10f; // World distance between samples, independent of FPS.
    float minSize = 0.12f, maxSize = 0.22f;
    float minLifetime = 0.35f, maxLifetime = 0.65f;
    float minDriftSpeed = 0.03f, maxDriftSpeed = 0.10f;
    std::uint32_t seed = 47;
};

// Read-only observer. Does not own meteors, simulate flight or render anything.
class MeteorTrailEmitter
{
public:
    explicit MeteorTrailEmitter(ParticleSystem& particles) : mParticles(particles) {}
    bool configure(const MeteorTrailConfig& config);
    void reset() { mStates.clear(); }
    // Register births BEFORE MeteorSystem::update; existing anchors stay intact.
    void observe(const std::vector<Meteor>& meteors);
    // AFTER flight and ParticleSystem::update: interpolate each surviving segment.
    // New particles include their subframe age; removed meteors stop emitting.
    void update(const std::vector<Meteor>& meteors, float deltaTime);
    std::size_t trackedCount() const { return mStates.size(); }
    const MeteorTrailConfig& config() const { return mConfig; }

private:
    struct State
    {
        glm::vec3 position{0};
        double remainder = 0;
        std::mt19937 random;
        bool seen = false;
    };
    static float random(State& state, float minimum, float maximum);
    ParticleSystem& mParticles;
    MeteorTrailConfig mConfig;
    std::unordered_map<MeteorId, State> mStates;
};
