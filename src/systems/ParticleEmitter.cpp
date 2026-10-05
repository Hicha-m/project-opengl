#include "systems/ParticleEmitter.h"
#include <algorithm>
#include <cmath>
#include <glm/gtc/constants.hpp>
namespace
{
    bool finite(const glm::vec3& v)
    {
        return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
    }
    bool range(float minimum, float maximum, bool zero = false)
    {
        return std::isfinite(minimum) && std::isfinite(maximum) && (zero ? minimum >= 0 : minimum > 0) &&
               maximum >= minimum;
    }
} // namespace
bool ParticleEmitter::configure(const ParticleBurstConfig& config)
{
    if (!std::isfinite(config.spreadRadians) || config.spreadRadians < 0 ||
        config.spreadRadians > glm::pi<float>() || !range(config.minSpeed, config.maxSpeed, true) ||
        !range(config.minSize, config.maxSize) || !range(config.minLifetime, config.maxLifetime))
    {
        return false;
    }
    mConfig = config;
    reset();
    return true;
}

float ParticleEmitter::random(float minimum, float maximum)
{
    const double unit = double(mRandom() >> 8) / 16777216.0;
    return float(double(minimum) + (double(maximum) - minimum) * unit);
}

bool ParticleEmitter::burst(const glm::vec3& position, const glm::vec3& direction)
{
    const double length = glm::length(glm::dvec3(direction));
    if (!finite(position) || !finite(direction) || length == 0)
    {
        return false;
    }
    const auto axis = glm::vec3(glm::dvec3(direction) / length);
    const auto helper = std::abs(axis.y) < 0.9f ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
    const auto tangent = glm::normalize(glm::cross(axis, helper));
    const auto bitangent = glm::cross(axis, tangent);
    for (std::size_t i = 0; i < mConfig.count; ++i)
    {
        const float cosine = random(std::cos(mConfig.spreadRadians), 1);
        const float sine = std::sqrt(std::max(0.0f, 1 - cosine * cosine));
        const float azimuth = random(0, glm::two_pi<float>());
        Particle particle;
        particle.position = position;
        particle.velocity =
            (axis * cosine + (tangent * std::cos(azimuth) + bitangent * std::sin(azimuth)) * sine) *
            random(mConfig.minSpeed, mConfig.maxSpeed);
        particle.size = random(mConfig.minSize, mConfig.maxSize);
        particle.lifetime = random(mConfig.minLifetime, mConfig.maxLifetime);
        mSystem.spawn(particle);
    }
    return true;
}
