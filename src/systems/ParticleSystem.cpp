#include "systems/ParticleSystem.h"
#include "graphics/ParticleRenderer.h"
#include <algorithm>
#include <cmath>

namespace
{
    bool finite(const glm::vec3& v)
    {
        return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
    }
} // namespace
ParticleSystem::ParticleSystem() = default;
ParticleSystem::~ParticleSystem() = default;
bool ParticleSystem::spawn(const Particle& particle)
{
    if (!finite(particle.position) || !finite(particle.velocity) || !std::isfinite(particle.size) ||
        particle.size <= 0 || !std::isfinite(particle.age) || particle.age < 0 ||
        !std::isfinite(particle.lifetime) || particle.lifetime <= particle.age)
    {
        return false;
    }
    mParticles.push_back(particle);
    return true;
}

void ParticleSystem::update(float deltaTime)
{
    if (!std::isfinite(deltaTime) || deltaTime <= 0)
    {
        return;
    }
    for (auto& particle : mParticles)
    {
        particle.position += particle.velocity * std::min(deltaTime, particle.lifetime - particle.age);
        particle.age = std::min(particle.lifetime, particle.age + deltaTime);
    }
    mParticles.erase(
        std::remove_if(mParticles.begin(), mParticles.end(), [](const Particle& particle)
                       { return particle.age >= particle.lifetime || !finite(particle.position); }),
        mParticles.end());
}

bool ParticleSystem::initGraphics()
{
    if (mRenderer)
    {
        return true;
    }
    auto renderer = std::make_unique<ParticleRenderer>();
    if (!renderer->init())
    {
        return false;
    }
    mRenderer = std::move(renderer);
    return true;
}

void ParticleSystem::releaseGraphics()
{
    mRenderer.reset();
}

void ParticleSystem::render(const glm::mat4& view, const glm::mat4& projection)
{
    if (mRenderer)
    {
        mRenderer->render(mParticles, view, projection);
    }
}
