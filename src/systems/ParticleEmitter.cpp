#include "systems/ParticleEmitter.h"
#include <cmath>
#include <algorithm>
#include <glm/gtc/constants.hpp>
namespace {
    bool finite(const glm::vec3& v) {
        return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
    }
    bool range(float a, float b, bool zero = false) {
        return std::isfinite(a) && std::isfinite(b) && (zero ? a >= 0 : a > 0) && b >= a;
    }
}
bool ParticleEmitter::configure(const ParticleBurstConfig& c)
{
    if (!std::isfinite(c.spreadRadians) || c.spreadRadians < 0 || c.spreadRadians > glm::pi<float>()
        || !range(c.minSpeed, c.maxSpeed, true) || !range(c.minSize, c.maxSize)
        || !range(c.minLifetime, c.maxLifetime)) return false;
    mConfig = c;
    reset();
    return true;
}
float ParticleEmitter::random(float a, float b)
{
    const double unit = double(mRandom() >> 8) / 16777216.0;
    return float(double(a) + (double(b) - a) * unit);
}
bool ParticleEmitter::burst(const glm::vec3& position, const glm::vec3& direction)
{
    const double length = glm::length(glm::dvec3(direction));
    if (!finite(position) || !finite(direction) || length == 0) return false;
    const auto axis = glm::vec3(glm::dvec3(direction) / length);
    const auto helper = std::abs(axis.y) < 0.9f ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
    const auto tangent = glm::normalize(glm::cross(axis, helper));
    const auto bitangent = glm::cross(axis, tangent);
    for (std::size_t i = 0; i < mConfig.count; ++i) {
        const float cosine = random(std::cos(mConfig.spreadRadians), 1);
        const float sine = std::sqrt(std::max(0.0f, 1 - cosine * cosine));
        const float azimuth = random(0, glm::two_pi<float>());
        Particle p;
        p.position = position;
        p.velocity = (axis * cosine + (tangent * std::cos(azimuth)
            + bitangent * std::sin(azimuth)) * sine) * random(mConfig.minSpeed, mConfig.maxSpeed);
        p.size = random(mConfig.minSize, mConfig.maxSize);
        p.lifetime = random(mConfig.minLifetime, mConfig.maxLifetime);
        mSystem.spawn(p);
    }
    return true;
}
