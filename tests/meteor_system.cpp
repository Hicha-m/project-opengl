#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include "systems/MeteorSystem.h"

static bool near(const glm::vec3& a, const glm::vec3& b)
{
    return glm::length(a - b) < 0.0001f;
}

int main()
{
    // No OpenGL context exists: instances and their lifetime are CPU data.
    MeteorSystem system;
    assert(system.size() == 0 && !system.graphicsReady());
    Transform first;
    first.position = {1, 2, 3};
    first.rotation = {0.2f, 0.3f, 0.4f};
    first.scale = {0.5f, 1, 2};
    Transform second;
    second.position = {-5, 0, 10};
    assert(system.spawn(first, {2, 0, -1}, 1));
    assert(system.spawn(second, {0, 4, 0}, 3));
    first.position = {100, 100, 100}; // Spawn takes a value, not a borrowed transform.
    system.update(0.5f);
    assert(near(system.meteors()[0].transform.position, {2, 2, 2.5f}));
    assert(near(system.meteors()[1].transform.position, {-5, 2, 10}));
    assert(near(system.meteors()[0].transform.rotation, {0.2f, 0.3f, 0.4f}));
    assert(near(system.meteors()[0].transform.scale, {0.5f, 1, 2}));
    assert(system.meteors()[0].lifetime == 0.5f);
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();
    for (float dt : {0.0f, -1.0f, nan, inf}) system.update(dt);
    assert(system.meteors()[0].lifetime == 0.5f);
    system.update(0.5f); // Expiry at exactly zero; remaining instance keeps its state.
    assert(system.size() == 1);
    assert(near(system.meteors()[0].transform.position, {-5, 4, 10}));
    system.update(10); // Frame crosses the remaining lifetime.
    assert(system.size() == 0);
    system.update(1);

    for (float lifetime : {0.0f, -1.0f, nan, inf})
        assert(!system.spawn(second, {0, 0, 0}, lifetime));
    assert(!system.spawn(second, {nan, 0, 0}, 1));
    second.scale.x = 0;
    assert(!system.spawn(second, {0, 0, 0}, 1));
    second.scale.x = 1;
    second.position.z = inf;
    assert(!system.spawn(second, {0, 0, 0}, 1));
    assert(system.size() == 0);

    for (int i = 0; i < 1000; ++i)
    {
        Transform transform;
        transform.position = {float(i), 0, 0};
        assert(system.spawn(transform, {0, float(i + 1), 0}, i % 2 ? 2 : 1));
    }
    system.update(1);
    assert(system.size() == 500);
    for (std::size_t i = 0; i < system.size(); ++i)
    {
        const float original = float(i * 2 + 1);
        assert(near(system.meteors()[i].transform.position, {original, original + 1, 0}));
    }
    system.clear();
    system.clear();
    assert(system.size() == 0);
    assert(system.spawn(Transform{}, {0, 0, 0}, 1));
    system.update(1);
    assert(system.size() == 0);
    std::cout << "Meteor population, independent motion and lifetime checks passed\n";
}
