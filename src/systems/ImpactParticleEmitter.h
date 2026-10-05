#pragma once
#include "systems/MeteorImpact.h"
#include "systems/ParticleEmitter.h"

// Only this adapter knows MeteorImpact. The generic emitter remains reusable.
class ImpactParticleEmitter
{
public:
    explicit ImpactParticleEmitter(ParticleSystem& system) : mEmitter(system) {}
    // Keep the cone inside the exterior hemisphere; also exposes the controlled seed.
    bool configure(const ParticleBurstConfig& config);
    void consume(const std::vector<MeteorImpact>& impacts);
    void reset() { mEmitter.reset(); }
    static constexpr float SurfaceOffset = 0.12f;

private:
    ParticleEmitter mEmitter;
};
