#pragma once
#include <cstdint>
#include <random>
#include "systems/MeteorSystem.h"

struct MeteorShowerConfig
{
    float spawnRate = 10.0f; // Instances per second; zero disables emission.
    glm::vec3 origin{0.0f};
    glm::vec3 spawnHalfExtents{1.0f}; // Axis-aligned box centered on origin.
    glm::vec3 direction{0.0f, -1.0f, 0.0f}; // Normalized internally.
    float spreadRadians = 0.15f; // Cone half-angle, in [0, pi].
    float minSpeed = 1.0f, maxSpeed = 3.0f;
    float minScale = 0.1f, maxScale = 0.3f;
    float minLifetime = 5.0f, maxLifetime = 10.0f;
    // Optional aimed bombardment: choose a point in a disk around target.
    bool aimed = false;
    glm::vec3 target{0};
    float targetRadius = 0;
    std::uint32_t seed = 42;
};

// Borrows a system which must outlive this generator. Owns no GPU resources.
class MeteorShower
{
public:
    explicit MeteorShower(MeteorSystem& system);
    MeteorShower(const MeteorShower&) = delete;
    MeteorShower& operator=(const MeteorShower&) = delete;

    // Valid configuration stops emission and resets seed/accumulator, not instances.
    // Invalid configuration leaves all generator state unchanged.
    bool configure(const MeteorShowerConfig& config);
    void reset(); // Restore configured emission, seed and credit; preserve instances.
    bool followTarget(const glm::vec3& target); // Translate the spawn zone; preserve RNG/credit.
    bool setEmission(float rate, float minScale, float maxScale); // Preserve RNG/credit.
    void start() { mRunning = true; } // Idempotent; resumes fractional spawn credit.
    void stop() { mRunning = false; }
    bool isRunning() const { return mRunning; }
    void update(float deltaTime); // Emission only; caller advances MeteorSystem separately.

private:
    float random(float minimum, float maximum);
    void emit();

    MeteorSystem& mSystem;
    MeteorShowerConfig mConfig;
    std::mt19937 mRandom;
    glm::vec3 mDirection{0.0f, -1.0f, 0.0f};
    glm::vec3 mTangent{1.0f, 0.0f, 0.0f};
    glm::vec3 mBitangent{0.0f, 0.0f, 1.0f};
    double mSpawnCredit = 0.0;
    bool mRunning = false;
    glm::vec3 mOrigin{0}, mTarget{0};
    float mRate = 0, mMinScale = 0, mMaxScale = 0;
};
