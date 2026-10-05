#pragma once
#include "systems/Particle.h"
#include <memory>
#include <vector>
class ParticleRenderer;

// Advances particles and removes expired ones; delegates drawing to ParticleRenderer.
class ParticleSystem
{
public:
    ParticleSystem();
    ~ParticleSystem();
    ParticleSystem(const ParticleSystem&) = delete;
    ParticleSystem& operator=(const ParticleSystem&) = delete;
    bool spawn(const Particle& particle);
    void update(float deltaTime);
    void clear() { mParticles.clear(); }
    std::size_t size() const { return mParticles.size(); }
    const std::vector<Particle>& particles() const { return mParticles; }
    // GPU lifecycle requires the owning context; CPU methods do not.
    bool initGraphics();
    void releaseGraphics();
    bool graphicsReady() const { return bool(mRenderer); }
    void render(const glm::mat4& view, const glm::mat4& projection);

private:
    std::vector<Particle> mParticles;
    std::unique_ptr<ParticleRenderer> mRenderer;
};
