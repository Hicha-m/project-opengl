#include "systems/ImpactLightSystem.h"
#include "scene/LightManager.h"
#include <algorithm>
#include <cmath>

namespace
{
    bool finite(const glm::vec3& v)
    {
        return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
    }
} // namespace

void ImpactLightSystem::consume(const std::vector<MeteorImpact>& impacts)
{
    for (const auto& impact : impacts)
    {
        const double normalLength = glm::length(glm::dvec3(impact.normal));
        if (!finite(impact.position) || !finite(impact.normal) || !finite(impact.velocity) ||
            normalLength == 0 || !std::isfinite(impact.meteorScale) || impact.meteorScale <= 0)
        {
            continue;
        }
        ImpactLight light;
        light.position =
            impact.position + glm::vec3(glm::dvec3(impact.normal) / normalLength) * SurfaceOffset;
        const double strength = glm::length(glm::dvec3(impact.velocity)) * impact.meteorScale * 3.0;
        light.initialIntensity = float(std::clamp(strength, 2.0, 30.0));
        light.intensity = light.initialIntensity;
        mLights.push_back(light);
    }
}

void ImpactLightSystem::update(float deltaTime)
{
    if (!std::isfinite(deltaTime) || deltaTime < 0)
    {
        return;
    }
    for (auto& light : mLights)
    {
        if (light.fresh)
        {
            light.fresh = false;
            continue;
        }
        light.age = std::min(light.lifetime, light.age + deltaTime);
        const float remaining = 1.0f - light.age / light.lifetime;
        light.intensity = light.initialIntensity * remaining * remaining;
    }
    mLights.erase(std::remove_if(mLights.begin(), mLights.end(),
                                 [](const ImpactLight& light) { return light.age >= light.lifetime; }),
                  mLights.end());
}

void ImpactLightSystem::clear()
{
    mLights.clear();
}

void ImpactLightSystem::publish(LightManager& manager) const
{
    std::vector<PointLight> points;
    points.reserve(mLights.size());
    for (const auto& light : mLights)
    {
        PointLight point;
        point.position = light.position;
        point.color = light.color;
        point.intensity = light.intensity;
        // Short range in world units: illuminate the impact neighborhood.
        point.linear = 1.0f;
        point.quadratic = 2.0f;
        points.push_back(point);
    }
    manager.setTransientPointLights(points);
}
