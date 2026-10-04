#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <glm/gtc/constants.hpp>
#include "systems/MeteorShower.h"

static bool same(const Meteor& a, const Meteor& b)
{
    return a.transform.position == b.transform.position
        && a.transform.rotation == b.transform.rotation
        && a.transform.scale == b.transform.scale
        && a.velocity == b.velocity && a.lifetime == b.lifetime;
}

static void identical(const MeteorSystem& a, const MeteorSystem& b)
{
    assert(a.size() == b.size());
    for (std::size_t i = 0; i < a.size(); ++i) assert(same(a.meteors()[i], b.meteors()[i]));
}

int main()
{
    MeteorSystem system;
    MeteorShower shower(system);
    MeteorShowerConfig config;
    config.origin = {3, 5, -2};
    config.spawnHalfExtents = {2, 3, 4};
    config.direction = {1, -4, 2}; // Must accept a non-unit direction.
    config.spreadRadians = 0.4f;
    assert(shower.configure(config));
    shower.update(1);
    assert(system.size() == 0 && !shower.isRunning());
    shower.start();
    shower.start();
    shower.update(0.0625f);
    assert(system.size() == 0);
    shower.stop();
    shower.update(10); // Stopped time must not accumulate emission debt.
    assert(system.size() == 0);
    shower.start();
    shower.update(0.0625f); // Preserve fractional credit on stop/start.
    assert(system.size() == 1);
    shower.update(0.875f);
    assert(system.size() == 10);
    shower.stop();
    const Meteor before = system.meteors()[0];
    shower.update(10);
    assert(system.size() == 10 && same(before, system.meteors()[0]));
    system.update(0.5f); // stop does not prevent independent instance simulation.
    assert(glm::length(system.meteors()[0].transform.position
        - (before.transform.position + before.velocity * 0.5f)) < 0.0001f);
    system.update(20);
    assert(system.size() == 0);

    assert(shower.configure(config));
    shower.start();
    shower.update(100);
    assert(system.size() == 1000); // Generator never updates/removes existing meteors.
    bool variedPosition = false, variedSpeed = false, variedScale = false;
    bool variedDirection = false, variedLifetime = false;
    const auto& first = system.meteors()[0];
    const glm::vec3 axis = glm::normalize(config.direction);
    for (const auto& meteor : system.meteors())
    {
        const glm::vec3 offset = meteor.transform.position - config.origin;
        for (int component = 0; component < 3; ++component)
            assert(std::abs(offset[component]) <= config.spawnHalfExtents[component]);
        const float speed = glm::length(meteor.velocity);
        assert(speed >= config.minSpeed - 0.00001f && speed <= config.maxSpeed + 0.00001f);
        assert(meteor.transform.scale.x >= config.minScale && meteor.transform.scale.x <= config.maxScale);
        assert(meteor.transform.scale == glm::vec3(meteor.transform.scale.x));
        assert(meteor.lifetime >= config.minLifetime && meteor.lifetime <= config.maxLifetime);
        assert(glm::dot(glm::normalize(meteor.velocity), axis) >= std::cos(config.spreadRadians) - 0.00001f);
        variedPosition |= meteor.transform.position != first.transform.position;
        variedSpeed |= std::abs(speed - glm::length(first.velocity)) > 0.001f;
        variedScale |= meteor.transform.scale != first.transform.scale;
        variedDirection |= glm::length(glm::normalize(meteor.velocity) - glm::normalize(first.velocity)) > 0.001f;
        variedLifetime |= meteor.lifetime != first.lifetime;
    }
    assert(variedPosition && variedSpeed && variedScale && variedDirection && variedLifetime);

    MeteorSystem reference;
    MeteorShower referenceShower(reference);
    assert(referenceShower.configure(config));
    referenceShower.start();
    referenceShower.update(100);
    identical(system, reference);
    auto otherSeed = config;
    otherSeed.seed = 43;
    MeteorSystem other;
    MeteorShower otherShower(other);
    assert(otherShower.configure(otherSeed));
    otherShower.start();
    otherShower.update(100);
    assert(!same(system.meteors()[0], other.meteors()[0]));
    system.clear();
    assert(shower.configure(config)); // Reconfigure replays the seed, without clearing the system.
    assert(!shower.isRunning());
    shower.start();
    shower.update(100);
    identical(system, reference);

    for (int fps : {30, 60, 144})
    {
        MeteorSystem stepped;
        MeteorShower steppedShower(stepped);
        assert(steppedShower.configure(config));
        steppedShower.start();
        for (int frame = 0; frame < 10 * fps; ++frame) steppedShower.update(1.0f / fps);
        assert(stepped.size() == 100);
        for (std::size_t i = 0; i < stepped.size(); ++i)
            assert(same(stepped.meteors()[i], reference.meteors()[i]));
    }

    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();
    MeteorSystem guarded, control;
    MeteorShower guardedShower(guarded), controlShower(control);
    assert(guardedShower.configure(config) && controlShower.configure(config));
    guardedShower.start();
    controlShower.start();
    guardedShower.update(0.0625f);
    controlShower.update(0.0625f);
    for (float dt : {0.0f, -1.0f, nan, inf}) guardedShower.update(dt);
    auto rejected = [&](const MeteorShowerConfig& invalid)
    {
        assert(!guardedShower.configure(invalid));
        assert(guardedShower.isRunning());
    };
    for (int field = 0; field < 15; ++field)
    {
        auto invalid = config;
        switch (field)
        {
        case 0: invalid.spawnRate = -1; break;
        case 1: invalid.spawnRate = inf; break;
        case 2: invalid.origin.x = nan; break;
        case 3: invalid.spawnHalfExtents.y = -1; break;
        case 4: invalid.spawnHalfExtents.z = inf; break;
        case 5: invalid.direction = glm::vec3(0); break;
        case 6: invalid.direction.x = inf; break;
        case 7: invalid.spreadRadians = -0.1f; break;
        case 8: invalid.spreadRadians = glm::pi<float>() + 0.1f; break;
        case 9: invalid.minSpeed = -1; break;
        case 10: invalid.maxSpeed = invalid.minSpeed - 0.1f; break;
        case 11: invalid.minScale = 0; break;
        case 12: invalid.maxScale = nan; break;
        case 13: invalid.minLifetime = 0; break;
        case 14: invalid.maxLifetime = invalid.minLifetime - 1; break;
        }
        rejected(invalid);
    }
    guardedShower.update(0.9375f);
    controlShower.update(0.9375f);
    identical(guarded, control); // Rejected inputs leave phase and RNG untouched.
    assert(guardedShower.configure(config));
    assert(guarded.size() == 10 && !guardedShower.isRunning());

    // Degenerate ranges and vertical axes are valid, including no dispersion.
    auto fixed = config;
    fixed.spawnHalfExtents = glm::vec3(0);
    fixed.direction = {0, -10, 0};
    fixed.spreadRadians = 0;
    fixed.minSpeed = fixed.maxSpeed = 2;
    fixed.minScale = fixed.maxScale = 0.5f;
    fixed.minLifetime = fixed.maxLifetime = 3;
    system.clear();
    assert(shower.configure(fixed));
    shower.start();
    shower.update(1);
    for (const auto& meteor : system.meteors())
    {
        assert(meteor.transform.position == fixed.origin);
        assert(meteor.velocity == glm::vec3(0, -2, 0));
        assert(meteor.transform.scale == glm::vec3(0.5f) && meteor.lifetime == 3);
    }
    fixed.spawnRate = 0;
    assert(shower.configure(fixed));
    shower.start();
    shower.update(100);
    assert(system.size() == 10);
    fixed.spawnRate = 10;
    fixed.minSpeed = fixed.maxSpeed = 0;
    assert(shower.configure(fixed));
    shower.start();
    shower.update(1);
    assert(system.meteors().back().velocity == glm::vec3(0));
    // Intensity changes preserve credit and RNG; reset restores authored defaults.
    system.clear(); assert(shower.configure(config)); shower.start(); shower.update(0.05f);
    assert(shower.setEmission(20,0.8f,1.2f)); shower.update(0.025f);
    assert(system.size()==1 && system.meteors()[0].transform.scale.x>=0.8f);
    assert(!shower.setEmission(-1,0.8f,1.2f) && !shower.setEmission(1,2,1));
    shower.reset(); system.clear(); shower.start(); shower.update(0.1f);
    assert(system.size()==1 && system.meteors()[0].transform.scale.x<=config.maxScale);
    MeteorShowerConfig aimed=config; aimed.aimed=true; aimed.origin={0,80,0};
    aimed.spawnHalfExtents={40,5,40}; aimed.target={0,0,0}; aimed.targetRadius=8;
    aimed.minSpeed=10; aimed.maxSpeed=15; aimed.minLifetime=20; aimed.maxLifetime=20;
    assert(shower.configure(aimed)); system.clear(); shower.start(); shower.update(10);
    const auto aimedBirths=system.meteors();
    for(const auto& meteor:aimedBirths) {
        assert(meteor.velocity.y<0);
        const auto d=glm::normalize(meteor.velocity);
        const auto relative=meteor.transform.position-aimed.target;
        assert(glm::length(relative-d*glm::dot(relative,d))<=aimed.targetRadius+0.0001f);
    }
    shower.reset(); system.clear(); shower.start(); shower.update(10);
    for(std::size_t i=0;i<aimedBirths.size();++i) assert(same(aimedBirths[i],system.meteors()[i]));
    aimed.target=aimed.origin; assert(!shower.configure(aimed)); // Reject ambiguous zero-length aim.
    std::cout << "Meteor shower cadence, configuration and determinism checks passed\n";
}
