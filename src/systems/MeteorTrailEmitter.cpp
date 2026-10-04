#include "systems/MeteorTrailEmitter.h"
#include <algorithm>
#include <cmath>

namespace
{
    bool finite(const glm::vec3& v)
    {
        return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
    }
    bool range(float a, float b, bool zero = false)
    {
        return std::isfinite(a) && std::isfinite(b) && (zero ? a >= 0 : a > 0) && b >= a;
    }
}

bool MeteorTrailEmitter::configure(const MeteorTrailConfig& config)
{
    if (!std::isfinite(config.spacing) || config.spacing <= 0
        || !range(config.minSize, config.maxSize)
        || !range(config.minLifetime, config.maxLifetime)
        || !range(config.minDriftSpeed, config.maxDriftSpeed, true)) return false;
    mConfig = config;
    reset();
    return true;
}

float MeteorTrailEmitter::random(State& state, float minimum, float maximum)
{
    const double unit = double(state.random() >> 8) / 16777216.0;
    return float(double(minimum) + (double(maximum) - minimum) * unit);
}

void MeteorTrailEmitter::observe(const std::vector<Meteor>& meteors)
{
    for (auto& entry : mStates) entry.second.seen = false;
    for (const auto& meteor : meteors)
    {
        if (!meteor.id || !finite(meteor.transform.position)) continue;
        auto result = mStates.try_emplace(meteor.id);
        auto& state = result.first->second;
        if (result.second)
        {
            state.position = meteor.transform.position;
            // Per-ID RNG: removing another meteor or reordering the vector cannot
            // change this meteor's sizes, lifetimes or drift speeds.
            std::seed_seq seed{mConfig.seed, std::uint32_t(meteor.id), std::uint32_t(meteor.id >> 32)};
            state.random.seed(seed);
        }
        state.seen = true;
    }
    for (auto it = mStates.begin(); it != mStates.end();)
        if (!it->second.seen) it = mStates.erase(it);
        else ++it;
}

void MeteorTrailEmitter::update(const std::vector<Meteor>& meteors, float dt)
{
    if (!std::isfinite(dt) || dt <= 0) return;
    observe(meteors);
    for (const auto& meteor : meteors)
    {
        auto found = mStates.find(meteor.id);
        if (found == mStates.end()) continue;
        auto& state = found->second;
        const glm::dvec3 start(state.position);
        const glm::dvec3 segment = glm::dvec3(meteor.transform.position) - start;
        const double distance = glm::length(segment);
        state.position = meteor.transform.position;
        if (distance == 0) continue; // Stationary meteors emit no trail.
        const glm::vec3 direction(segment / distance);
        const double spacing = mConfig.spacing;
        // Small tolerance absorbs float position rounding at a sample boundary.
        const double tolerance = spacing * 0.00001;
        double offset = spacing - state.remainder;
        while (offset <= distance + tolerance)
        {
            const double fraction = std::min(1.0, offset / distance);
            Particle particle;
            particle.position = glm::vec3(start + segment * fraction);
            particle.velocity = -direction * random(state, mConfig.minDriftSpeed, mConfig.maxDriftSpeed);
            particle.size = random(state, mConfig.minSize, mConfig.maxSize);
            particle.lifetime = random(state, mConfig.minLifetime, mConfig.maxLifetime);
            particle.age = float(double(dt) * (1.0 - fraction));
            // Samples born earlier in this frame have already moved and faded.
            // Still advance RNG for expired samples to preserve subsequent emission.
            particle.position += particle.velocity * particle.age;
            mParticles.spawn(particle); // Rejects samples already expired.
            offset += spacing;
        }
        state.remainder = std::max(0.0, distance - (offset - spacing));
    }
}
