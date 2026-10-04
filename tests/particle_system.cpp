#include <cassert>
#include <cmath>
#include <limits>
#include <iostream>
#include "systems/ImpactParticleEmitter.h"

static void same(const std::vector<Particle>& a, const std::vector<Particle>& b)
{
    assert(a.size() == b.size());
    for (std::size_t i = 0; i < a.size(); ++i) {
        assert(a[i].position == b[i].position && a[i].velocity == b[i].velocity);
        assert(a[i].size == b[i].size && a[i].age == b[i].age && a[i].lifetime == b[i].lifetime);
    }
}
int main()
{
    ParticleSystem system;
    Particle p;
    p.position = {1,2,3}; p.velocity = {2,-1,0}; p.lifetime = 2;
    assert(system.spawn(p));
    system.update(0.5f);
    assert(system.particles()[0].position == glm::vec3(2,1.5f,3));
    assert(system.particles()[0].age == 0.5f);
    const auto snapshot = system.particles();
    system.update(-1); system.update(std::numeric_limits<float>::quiet_NaN());
    same(snapshot, system.particles());
    system.update(1.5f); assert(system.size() == 0);
    p.size = 0; assert(!system.spawn(p)); p.size = 1;
    p.age = 2; assert(!system.spawn(p)); p.age = 0;
    p.velocity.x = std::numeric_limits<float>::infinity(); assert(!system.spawn(p));
    ParticleEmitter emitter(system);
    ParticleBurstConfig config;
    config.seed = 123;
    for (std::size_t count : {100,1000,10000}) {
        config.count = count; assert(emitter.configure(config));
        assert(emitter.burst({2,3,4}, {1,2,3})); assert(system.size() == count);
        const auto axis = glm::normalize(glm::vec3(1,2,3));
        for (const auto& particle : system.particles()) {
            assert(particle.position == glm::vec3(2,3,4) && particle.age == 0);
            assert(glm::dot(glm::normalize(particle.velocity), axis) >= std::cos(config.spreadRadians) - 0.00001f);
            assert(glm::length(particle.velocity) >= config.minSpeed - 0.00001f);
            assert(glm::length(particle.velocity) <= config.maxSpeed + 0.00001f);
            assert(particle.size >= config.minSize && particle.size <= config.maxSize);
            assert(particle.lifetime >= config.minLifetime && particle.lifetime <= config.maxLifetime);
        }
        const auto first = system.particles();
        system.clear(); emitter.reset(); assert(emitter.burst({2,3,4}, {1,2,3}));
        same(first, system.particles());
        system.update(0.1f); assert(system.size() == count);
        system.update(2); assert(system.size() == 0);
    }
    config.count = 100; assert(emitter.configure(config));
    emitter.burst({0,0,0}, {0,1,0}); const auto seedA = system.particles();
    system.clear(); ++config.seed; assert(emitter.configure(config));
    emitter.burst({0,0,0}, {0,1,0});
    assert(system.particles()[0].velocity != seedA[0].velocity);
    system.clear();
    config.spreadRadians = -1; assert(!emitter.configure(config));
    assert(!emitter.burst({0,0,0}, {0,0,0})); assert(system.size() == 0);
    ImpactParticleEmitter impactEmitter(system);
    ParticleBurstConfig impactConfig; impactConfig.seed = 99;
    assert(impactEmitter.configure(impactConfig));
    impactConfig.spreadRadians = 2; assert(!impactEmitter.configure(impactConfig));
    std::vector<MeteorImpact> impacts = {
        {{1,2,3}, {0,2,0}, {0,-10,0}, 0.5f},
        {{-1,0,4}, {1,0,0}, {-10,0,0}, 1}};
    impactEmitter.consume(impacts); assert(system.size() == 96);
    for (std::size_t i = 0; i < system.size(); ++i) {
        const auto& impact = impacts[i / 48]; const auto& particle = system.particles()[i];
        assert(particle.position == impact.position + glm::normalize(impact.normal) * ImpactParticleEmitter::SurfaceOffset);
        assert(glm::dot(particle.velocity, impact.normal) > 0);
    }
    auto first = system.particles();
    system.clear(); impactEmitter.reset(); impactEmitter.consume(impacts); same(first, system.particles());
    system.clear(); impactEmitter.consume({{{0,0,0},{0,0,0},{0,0,0},1}}); assert(system.size() == 0);
    assert(!system.graphicsReady()); // All CPU behavior works without a GL context.
    std::cout << "Particle simulation, cone bursts, 100/1000/10000 populations and deterministic reset passed\n";
}
