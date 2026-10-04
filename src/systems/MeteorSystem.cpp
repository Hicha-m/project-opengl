#include "systems/MeteorSystem.h"
#include <algorithm>
#include <cmath>
#include "systems/MeteorResources.h"
#include "graphics/Renderer.h"

namespace
{
    bool finite(const glm::vec3& v)
    {
        return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
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
    mMeteors.push_back({transform, velocity, lifetime});
    return true;
}

void MeteorSystem::update(float deltaTime)
{
    if (!std::isfinite(deltaTime) || deltaTime <= 0) return;
    for (auto& meteor : mMeteors)
    {
        meteor.transform.position += meteor.velocity * std::min(deltaTime, meteor.lifetime);
        meteor.lifetime -= deltaTime;
    }
    mMeteors.erase(std::remove_if(mMeteors.begin(), mMeteors.end(),
        [](const Meteor& meteor) { return meteor.lifetime <= 0; }), mMeteors.end());
}

void MeteorSystem::clear()
{
    mMeteors.clear();
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
    const glm::mat4& view, const glm::mat4& projection, const glm::vec3& cameraPosition)
{
    if (!mResources) return;
    for (const auto& meteor : mMeteors)
        renderer.renderMesh(mResources->sphere.getMesh(), mResources->material,
            meteor.transform, lights, view, projection, cameraPosition);
}
