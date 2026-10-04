#include "systems/ParticleSystem.h"
#include "graphics/ParticleRenderer.h"
#include <algorithm>
#include <cmath>

namespace {
    bool finite(const glm::vec3& v) {
        return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
    }
}
ParticleSystem::ParticleSystem() = default;
ParticleSystem::~ParticleSystem() = default;
bool ParticleSystem::spawn(const Particle& p)
{
    if (!finite(p.position) || !finite(p.velocity) || !std::isfinite(p.size) || p.size <= 0
        || !std::isfinite(p.age) || p.age < 0 || !std::isfinite(p.lifetime)
        || p.lifetime <= p.age) return false;
    mParticles.push_back(p);
    return true;
}
void ParticleSystem::update(float dt)
{
    if (!std::isfinite(dt) || dt <= 0) return;
    for (auto& p : mParticles) {
        p.position += p.velocity * std::min(dt, p.lifetime - p.age);
        p.age = std::min(p.lifetime, p.age + dt);
    }
    mParticles.erase(std::remove_if(mParticles.begin(), mParticles.end(),
        [](const Particle& p) { return p.age >= p.lifetime || !finite(p.position); }), mParticles.end());
}
bool ParticleSystem::initGraphics()
{
    if (mRenderer) return true;
    auto renderer = std::make_unique<ParticleRenderer>();
    if (!renderer->init()) return false;
    mRenderer = std::move(renderer);
    return true;
}
void ParticleSystem::releaseGraphics() { mRenderer.reset(); }
void ParticleSystem::render(const glm::mat4& view, const glm::mat4& projection)
{
    if (mRenderer) mRenderer->render(mParticles, view, projection);
}
