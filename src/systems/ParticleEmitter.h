#pragma once
#include <cstdint>
#include <random>
#include "systems/ParticleSystem.h"

struct ParticleBurstConfig
{
    std::size_t count = 48;
    float spreadRadians = 1.3f;
    float minSpeed = 0.8f, maxSpeed = 3.0f;
    float minSize = 0.06f, maxSize = 0.16f;
    float minLifetime = 0.6f, maxLifetime = 1.4f;
    std::uint32_t seed = 46;
};
// Generic cone burst; no knowledge of the event that requested it.
class ParticleEmitter
{
public:
    explicit ParticleEmitter(ParticleSystem& system) : mSystem(system) { reset(); }
    bool configure(const ParticleBurstConfig& config);
    void reset() { mRandom.seed(mConfig.seed); }
    bool burst(const glm::vec3& position, const glm::vec3& direction);
    const ParticleBurstConfig& config() const { return mConfig; }
private:
    float random(float minimum, float maximum);
    ParticleSystem& mSystem;
    ParticleBurstConfig mConfig;
    std::mt19937 mRandom;
};
