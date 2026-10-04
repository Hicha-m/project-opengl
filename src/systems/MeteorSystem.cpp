#include "systems/MeteorSystem.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include "systems/MeteorResources.h"
#include "graphics/Renderer.h"

namespace
{
    bool finite(const glm::vec3& v)
    {
        return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
    }

    // Swept unit mesh bounding sphere vs target: first contact on the segment.
    bool contact(const Meteor& meteor, const glm::dvec3& step,
        const SphereCollider& collider, double& fraction, glm::vec3& normal)
    {
        const glm::dvec3 relative = glm::dvec3(meteor.transform.position) - glm::dvec3(collider.center);
        const double radius = double(collider.radius)
            + std::max({meteor.transform.scale.x, meteor.transform.scale.y, meteor.transform.scale.z});
        fraction = 0;
        if (glm::dot(relative, relative) > radius * radius)
        {
            const double lengthSquared = glm::dot(step, step);
            const double projection = glm::dot(relative, step);
            if (lengthSquared == 0 || projection >= 0) return false;
            const double closestTime = -projection / lengthSquared;
            const glm::dvec3 closest = relative + step * closestTime;
            const double distanceSquared = glm::dot(closest, closest);
            if (distanceSquared > radius * radius) return false;
            fraction = closestTime - std::sqrt((radius * radius - distanceSquared) / lengthSquared);
            if (fraction < 0 || fraction > 1) return false;
        }
        glm::dvec3 outward = relative + step * fraction;
        if (glm::dot(outward, outward) == 0)
        {
            // Initial overlap at target center: opposite motion, or +Y if stationary.
            outward = -glm::dvec3(meteor.velocity);
            if (glm::dot(outward, outward) == 0) outward = glm::dvec3(0, 1, 0);
        }
        normal = glm::vec3(glm::normalize(outward));
        return true;
    }
}

MeteorSystem::MeteorSystem() = default;
MeteorSystem::~MeteorSystem() = default;

bool MeteorSystem::spawn(const Transform& transform, const glm::vec3& velocity, float lifetime)
{
    if (!std::isfinite(lifetime) || lifetime <= 0 || !finite(velocity)
        || !finite(transform.position) || !finite(transform.rotation) || !finite(transform.scale)
        || transform.scale.x <= 0 || transform.scale.y <= 0 || transform.scale.z <= 0)
        return false;
    if (mNextId == 0) return false; // Do not reuse IDs after unsigned overflow.
    mMeteors.push_back({transform, velocity, lifetime, mNextId});
    ++mNextId;
    return true;
}

void MeteorSystem::update(float deltaTime)
{
    simulate(deltaTime, nullptr);
}

void MeteorSystem::update(float deltaTime, const SphereCollider& collider)
{
    simulate(deltaTime, &collider);
}

void MeteorSystem::simulate(float deltaTime, const SphereCollider* collider)
{
    mImpacts.clear();
    if (!std::isfinite(deltaTime) || deltaTime <= 0) return;
    if (collider && !collider->isValid()) throw std::invalid_argument("Invalid sphere collider");
    for (auto& meteor : mMeteors)
    {
        const double duration = std::min(deltaTime, meteor.lifetime);
        const glm::dvec3 step = glm::dvec3(meteor.velocity) * duration;
        double fraction;
        glm::vec3 normal;
        if (collider && contact(meteor, step, *collider, fraction, normal)
            && duration * fraction < meteor.lifetime) // Expiry wins an exact tie.
        {
            mImpacts.push_back({collider->center + normal * collider->radius, normal,
                meteor.velocity, std::max({meteor.transform.scale.x,
                    meteor.transform.scale.y, meteor.transform.scale.z})});
            meteor.lifetime = 0;
            continue;
        }
        meteor.transform.position += glm::vec3(step);
        meteor.lifetime -= deltaTime;
    }
    mMeteors.erase(std::remove_if(mMeteors.begin(), mMeteors.end(),
        [](const Meteor& meteor) { return meteor.lifetime <= 0; }), mMeteors.end());
}

void MeteorSystem::clear()
{
    mMeteors.clear();
    mNextId = 1;
    mImpacts.clear();
}

bool MeteorSystem::initGraphics()
{
    if (mResources) return true;
    auto resources = std::make_unique<MeteorResources>();
    if (!resources->load()) return false;
    mResources = std::move(resources);
    return true;
}

void MeteorSystem::releaseGraphics()
{
    mResources.reset();
}

void MeteorSystem::render(Renderer& renderer, LightManager& lights,
    const glm::mat4& view, const glm::mat4& projection, const glm::vec3& cameraPosition,
    const SphereCollider* absorption)
{
    if (!mResources) return;
    for (const auto& meteor : mMeteors) {
        auto visible = meteor.transform;
        if (absorption && absorption->isValid()) {
            const float size = std::max({visible.scale.x,visible.scale.y,visible.scale.z});
            const float contactRadius = absorption->radius + size;
            const float outerRadius = contactRadius + 2.0f * absorption->radius;
            const float distance = glm::distance(visible.position,absorption->center);
            const float progress = glm::clamp((distance-contactRadius)/(outerRadius-contactRadius),0.0f,1.0f);
            const float factor = progress*progress*(3.0f-2.0f*progress);
            if (factor <= 0) continue;
            visible.scale *= factor;
        }
        renderer.renderMesh(mResources->sphere.getMesh(), mResources->material,
            visible, lights, view, projection, cameraPosition);
    }
}
