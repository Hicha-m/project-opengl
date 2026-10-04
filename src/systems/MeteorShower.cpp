#include "systems/MeteorShower.h"
#include <cmath>
#include <algorithm>
#include <glm/gtc/constants.hpp>

namespace
{
    bool finite(const glm::vec3& value)
    {
        return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
    }

    bool range(float minimum, float maximum, bool allowZero = false)
    {
        return std::isfinite(minimum) && std::isfinite(maximum)
            && (allowZero ? minimum >= 0 : minimum > 0) && maximum >= minimum;
    }
}

MeteorShower::MeteorShower(MeteorSystem& system) : mSystem(system)
{
    configure(MeteorShowerConfig{});
}

bool MeteorShower::configure(const MeteorShowerConfig& config)
{
    // Use double for normalization so finite, very small/large directions remain valid.
    const double length = glm::length(glm::dvec3(config.direction));
    const auto& extents = config.spawnHalfExtents;
    if (!std::isfinite(config.spawnRate) || config.spawnRate < 0
        || !finite(config.origin) || !finite(extents)
        || extents.x < 0 || extents.y < 0 || extents.z < 0
        || !finite(config.direction) || length == 0
        || !std::isfinite(config.spreadRadians) || config.spreadRadians < 0
        || config.spreadRadians > glm::pi<float>()
        || !range(config.minSpeed, config.maxSpeed, true)
        || !range(config.minScale, config.maxScale)
        || !range(config.minLifetime, config.maxLifetime)
        || !finite(config.target) || !std::isfinite(config.targetRadius) || config.targetRadius < 0
        || (config.aimed && glm::length(glm::max(glm::abs(glm::dvec3(config.target)-glm::dvec3(config.origin))-glm::dvec3(extents),glm::dvec3(0))) <= config.targetRadius))
        return false;
    // Reject boxes whose endpoints overflow the float positions used by MeteorSystem.
    if (!finite(config.origin - extents) || !finite(config.origin + extents)) return false;

    mConfig = config;
    mDirection = glm::vec3(glm::dvec3(config.direction) / length);
    const glm::vec3 helper = std::abs(mDirection.y) < 0.9f
        ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
    mTangent = glm::normalize(glm::cross(mDirection, helper));
    mBitangent = glm::cross(mDirection, mTangent);
    reset();
    return true;
}

void MeteorShower::reset()
{
    mOrigin=mConfig.origin; mTarget=mConfig.target;
    mRate = mConfig.spawnRate; mMinScale = mConfig.minScale; mMaxScale = mConfig.maxScale;
    mRandom.seed(mConfig.seed);
    mSpawnCredit = 0;
    mRunning = false;
}

bool MeteorShower::followTarget(const glm::vec3& target)
{
    const auto origin=mOrigin+(target-mTarget);
    if(!finite(target) || !finite(origin-mConfig.spawnHalfExtents) || !finite(origin+mConfig.spawnHalfExtents)) return false;
    mOrigin=origin; mTarget=target; return true;
}

bool MeteorShower::setEmission(float rate, float minScale, float maxScale)
{
    if (!std::isfinite(rate) || rate < 0 || !range(minScale,maxScale)) return false;
    mRate=rate; mMinScale=minScale; mMaxScale=maxScale;
    return true;
}

float MeteorShower::random(float minimum, float maximum)
{
    // Fixed mapping from mt19937 bits: avoids library-dependent real distributions.
    const double unit = double(mRandom() >> 8) / 16777216.0;
    return float(double(minimum) + (double(maximum) - minimum) * unit);
}

void MeteorShower::emit()
{
    Transform transform;
    const auto& extents = mConfig.spawnHalfExtents;
    for (int component = 0; component < 3; ++component)
        transform.position[component] = mOrigin[component]
            + random(-extents[component], extents[component]);
    transform.scale = glm::vec3(random(mMinScale, mMaxScale));
    // Uniform solid-angle sampling inside a cone around the general direction.
    const float cosine = random(std::cos(mConfig.spreadRadians), 1.0f);
    const float sine = std::sqrt(std::max(0.0f, 1.0f - cosine * cosine));
    const float azimuth = random(0, glm::two_pi<float>());
    glm::vec3 direction = glm::normalize(mDirection * cosine
        + (mTangent * std::cos(azimuth) + mBitangent * std::sin(azimuth)) * sine);
    if (mConfig.aimed) {
        const auto incoming = glm::normalize(mTarget-transform.position);
        const auto helper = std::abs(incoming.y) < 0.9f ? glm::vec3(0,1,0) : glm::vec3(1,0,0);
        const auto tangent = glm::normalize(glm::cross(incoming,helper));
        const auto bitangent = glm::cross(incoming,tangent);
        const float radius = std::sqrt(random(0,1))*mConfig.targetRadius;
        const float angle = random(0,glm::two_pi<float>());
        const auto aim = mTarget + radius*(tangent*std::cos(angle)+bitangent*std::sin(angle));
        direction = glm::normalize(aim-transform.position);
    }
    const float speed = random(mConfig.minSpeed, mConfig.maxSpeed);
    const float lifetime = random(mConfig.minLifetime, mConfig.maxLifetime);
    mSystem.spawn(transform, direction * speed, lifetime);
}

void MeteorShower::update(float deltaTime)
{
    if (!mRunning || !std::isfinite(deltaTime) || deltaTime <= 0) return;
    mSpawnCredit += double(deltaTime) * mRate;
    while (mSpawnCredit >= 1.0)
    {
        emit();
        mSpawnCredit -= 1.0;
    }
}
