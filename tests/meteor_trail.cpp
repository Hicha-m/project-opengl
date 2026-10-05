#include "systems/MeteorSystem.h"
#include "systems/MeteorTrailEmitter.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>

static void same(const std::vector<Particle>& a, const std::vector<Particle>& b, float tolerance = 0)
{
    assert(a.size() == b.size());
    for (std::size_t i = 0; i < a.size(); ++i)
    {
        assert(glm::length(a[i].position - b[i].position) <= tolerance);
        assert(glm::length(a[i].velocity - b[i].velocity) <= tolerance);
        assert(a[i].size == b[i].size && a[i].lifetime == b[i].lifetime);
        assert(std::abs(a[i].age - b[i].age) <= tolerance);
    }
}

static std::vector<Particle> trajectory(int fps, float lifetime, bool simulateParticles)
{
    MeteorSystem meteors;
    ParticleSystem particles;
    MeteorTrailEmitter trail(particles);
    MeteorTrailConfig config;
    config.spacing = 0.25f;
    config.minLifetime = config.maxLifetime = lifetime;
    config.minDriftSpeed = config.maxDriftSpeed = 0;
    assert(trail.configure(config));
    assert(meteors.spawn(Transform{}, {4, 0, 0}, 10));
    trail.observe(meteors.meteors());
    for (int frame = 0; frame < fps; ++frame)
    {
        const float dt = 1.0f / fps;
        meteors.update(dt);
        if (simulateParticles)
        {
            particles.update(dt);
        }
        trail.update(meteors.meteors(), dt);
    }
    assert(trail.trackedCount() == 1);
    return particles.particles();
}

int main()
{
    // Spatial samples, including multiple emissions within one slow frame.
    auto reference = trajectory(30, 10, false);
    assert(reference.size() == 16);
    for (std::size_t i = 0; i < reference.size(); ++i)
    {
        assert(std::abs(reference[i].position.x - float(i + 1) * 0.25f) < 0.00001f);
    }
    for (int fps : {1, 60, 144})
    {
        auto samples = trajectory(fps, 10, false);
        assert(samples.size() == reference.size());
        for (std::size_t i = 0; i < samples.size(); ++i)
        {
            assert(glm::distance(samples[i].position, reference[i].position) < 0.00001f);
            assert(samples[i].size == reference[i].size && samples[i].lifetime == reference[i].lifetime);
        }
    }
    // Short lifetime: same visible tail at 30/60/144 FPS, also one large frame.
    auto visible = trajectory(30, 0.55f, true);
    assert(visible.size() == 9);
    for (int fps : {1, 60, 144})
    {
        same(visible, trajectory(fps, 0.55f, true), 0.00002f);
    }

    MeteorSystem meteors;
    ParticleSystem particles;
    MeteorTrailEmitter trail(particles);
    MeteorTrailConfig config;
    config.spacing = 0.25f;
    config.minLifetime = config.maxLifetime = 2;
    config.minDriftSpeed = config.maxDriftSpeed = 0;
    assert(trail.configure(config));
    Transform transform;
    assert(meteors.spawn(transform, {1, 0, 0}, 0.5f));
    transform.position.y = 5;
    assert(meteors.spawn(transform, {1, 0, 0}, 10));
    const auto survivorId = meteors.meteors()[1].id;
    assert(survivorId != meteors.meteors()[0].id);
    trail.observe(meteors.meteors());
    meteors.update(0.125f);
    trail.update(meteors.meteors(), 0.125f);
    assert(particles.size() == 0); // Residual distance retained independently.
    meteors.update(0.125f);
    trail.update(meteors.meteors(), 0.125f);
    assert(particles.size() == 2);
    meteors.update(0.25f);
    trail.update(meteors.meteors(), 0.25f);
    assert(meteors.size() == 1 && meteors.meteors()[0].id == survivorId);
    assert(trail.trackedCount() == 1 && particles.size() == 3);
    assert(particles.particles().back().position == glm::vec3(0.5f, 5, 0));
    // Vector reallocation cannot disturb state; new stationary objects do not emit.
    for (int i = 0; i < 1000; ++i)
    {
        assert(meteors.spawn(transform, {0, 0, 0}, 10));
    }
    trail.observe(meteors.meteors());
    meteors.update(0.25f);
    trail.update(meteors.meteors(), 0.25f);
    assert(particles.size() == 4 && trail.trackedCount() == 1001);
    const auto before = particles.particles();
    for (float dt : {0.0f, -1.0f, std::numeric_limits<float>::quiet_NaN()})
    {
        trail.update(meteors.meteors(), dt);
    }
    same(before, particles.particles());
    config.spacing = 0;
    assert(!trail.configure(config));
    config.spacing = 0.25f;
    config.maxLifetime = -1;
    assert(!trail.configure(config));
    meteors.clear();
    trail.update(meteors.meteors(), 0.1f);
    assert(trail.trackedCount() == 0); // Existing particles remain, then expire normally.
    particles.update(3);
    assert(particles.size() == 0);

    // Full sequence reset reproduces IDs, RNG and particle data exactly.
    std::vector<Particle> firstPass;
    for (int pass = 0; pass < 2; ++pass)
    {
        meteors.clear();
        particles.clear();
        trail.reset();
        assert(trail.trackedCount() == 0);
        assert(meteors.spawn(Transform{}, {3, 0, 0}, 10));
        assert(meteors.meteors()[0].id == 1);
        trail.observe(meteors.meteors());
        for (int frame = 0; frame < 8; ++frame)
        {
            meteors.update(0.125f);
            particles.update(0.125f);
            trail.update(meteors.meteors(), 0.125f);
        }
        if (!pass)
        {
            firstPass = particles.particles();
        }
        else
        {
            same(firstPass, particles.particles());
        }
    }
    // Per-ID randomness does not depend on the presence or order of other meteors.
    auto active = meteors.meteors();
    active[0].id = 100;
    active[0].transform.position = {0, 0, 0};
    Meteor other = active[0];
    other.id = 200;
    other.transform.position.y = 50;
    ParticleSystem isolated, combined;
    MeteorTrailEmitter one(isolated), two(combined);
    one.observe(active);
    two.observe({other, active[0]});
    active[0].transform.position.x = 1;
    other.transform.position.x = 1;
    one.update(active, 0.1f);
    two.update({active[0], other}, 0.1f);
    std::vector<Particle> selected;
    for (const auto& particle : combined.particles())
    {
        if (particle.position.y == 0)
        {
            selected.push_back(particle);
        }
    }
    same(isolated.particles(), selected);
    auto seedA = isolated.particles();
    isolated.clear();
    one.reset();
    MeteorTrailConfig alternate;
    alternate.seed = 999;
    assert(one.configure(alternate));
    active[0].transform.position.x = 0;
    one.observe(active);
    active[0].transform.position.x = 1;
    one.update(active, 0.1f);
    assert(seedA[0].size != isolated.particles()[0].size);
    std::cout << "Meteor IDs, spatial trail density, short lifetime, removal and replay checks passed\n";
}
