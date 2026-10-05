#include "systems/ImpactParticleEmitter.h"
#include <cmath>
#include <glm/gtc/constants.hpp>
bool ImpactParticleEmitter::configure(const ParticleBurstConfig& config)
{
    return config.spreadRadians <= glm::half_pi<float>() && mEmitter.configure(config);
}

void ImpactParticleEmitter::consume(const std::vector<MeteorImpact>& impacts)
{
    for (const auto& impact : impacts)
    {
        const double length = glm::length(glm::dvec3(impact.normal));
        if (!std::isfinite(length) || length == 0 || !std::isfinite(impact.meteorScale) ||
            impact.meteorScale <= 0)
        {
            continue;
        }
        const auto normal = glm::vec3(glm::dvec3(impact.normal) / length);
        mEmitter.burst(impact.position + normal * SurfaceOffset, normal);
    }
}
