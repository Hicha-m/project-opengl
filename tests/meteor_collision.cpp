#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include "systems/MeteorSystem.h"

static bool near(const glm::vec3& a, const glm::vec3& b)
{
    return glm::length(a - b) < 0.0001f;
}

int main()
{
    MeteorSystem system;
    const SphereCollider sphere{{0, 0, 0}, 2};
    auto spawn = [&](glm::vec3 position, glm::vec3 velocity, float lifetime = 10, float scale = 1)
    {
        Transform transform;
        transform.position = position;
        transform.scale = glm::vec3(scale);
        assert(system.spawn(transform, velocity, lifetime));
    };
    auto check = [&](const MeteorImpact& impact)
    {
        assert(std::abs(glm::length(impact.position - sphere.center) - sphere.radius) < 0.0001f);
        assert(std::abs(glm::length(impact.normal) - 1) < 0.0001f);
        assert(near(impact.position, sphere.center + impact.normal * sphere.radius));
    };
    spawn({-5, 0, 0}, {1, 0, 0});
    system.update(1, sphere);
    assert(system.size() == 1 && system.impacts().empty());
    system.update(1, sphere); // Meteor's front touches; center is still outside the target.
    assert(system.size() == 0 && system.impacts().size() == 1);
    const auto slow = system.impacts()[0];
    assert(near(slow.position, {-2, 0, 0}) && near(slow.normal, {-1, 0, 0}));
    assert(slow.velocity == glm::vec3(1, 0, 0) && slow.meteorScale == 1);
    check(slow);
    system.update(1, sphere);
    assert(system.impacts().empty()); // No double impact.

    spawn({-1000, 0, 0}, {200000, 0, 0});
    system.update(0.1f, sphere); // Both segment endpoints outside, high speed.
    assert(system.size() == 0 && system.impacts().size() == 1);
    assert(near(system.impacts()[0].position, {-2, 0, 0}));
    check(system.impacts()[0]);
    spawn({-10, 0, 0}, {1, 0, 0}, 100);
    system.update(30, sphere); // Large dt: detect the entry, not the far-side exit.
    assert(system.size() == 0 && system.impacts().size() == 1);
    assert(near(system.impacts()[0].normal, {-1, 0, 0}));

    spawn({-5, 3.01f, 0}, {10, 0, 0});
    spawn({-5, 0, 0}, {-10, 0, 0}); // Moving away.
    spawn({-5, 0, 0}, {0, 0, 0}); // Stationary outside.
    system.update(1, sphere);
    assert(system.size() == 3 && system.impacts().empty());
    system.clear();
    spawn({-5, 3, 0}, {10, 0, 0}); // Exact tangent to the expanded sphere.
    system.update(1, sphere);
    assert(system.size() == 0 && system.impacts().size() == 1);
    assert(near(system.impacts()[0].position, {0, 2, 0}));
    check(system.impacts()[0]);

    spawn({-5, 0, 0}, {10, 0, 0}, 0.1f); // Expires before contact at 0.2 s.
    system.update(1, sphere);
    assert(system.size() == 0 && system.impacts().empty());
    spawn({-5, 0, 0}, {1, 0, 0}, 2); // Exact expiry/contact tie: expiry wins.
    system.update(4, sphere);
    assert(system.size() == 0 && system.impacts().empty());
    spawn({-5, 0, 0}, {1, 0, 0}, 3);
    system.update(4, sphere); // Hit before expiry, although dt crosses both.
    assert(system.size() == 0 && system.impacts().size() == 1);

    spawn({-5, 0, 0}, {10, 0, 0}, 10, 0.5f);
    spawn({0, 5, 0}, {0, -10, 0}, 10, 1.5f);
    spawn({0, 0, -5}, {0, 0, 10});
    spawn({10, 10, 10}, {0, 0, 0});
    system.update(1, sphere);
    assert(system.size() == 1 && system.impacts().size() == 3);
    for (const auto& impact : system.impacts()) check(impact);
    assert(system.impacts()[0].meteorScale == 0.5f);
    assert(system.impacts()[1].meteorScale == 1.5f);
    assert(near(system.impacts()[1].position, {0, 2, 0}));
    system.update(1); // No-collider update clears frame impacts too.
    assert(system.size() == 1 && system.impacts().empty());
    system.clear();

    spawn({1, 0, 0}, {0, 0, 0}); // Initial overlap: immediate contact projected to surface.
    spawn({0, 0, 0}, {0, 0, 0}); // Stable fallback normal at the exact center.
    system.update(0.1f, sphere);
    assert(system.size() == 0 && system.impacts().size() == 2);
    check(system.impacts()[0]);
    check(system.impacts()[1]);
    assert(near(system.impacts()[1].normal, {0, 1, 0}));
    system.clear();
    assert(system.impacts().empty());

    const SphereCollider translated{{30, 50, -10}, 10};
    spawn({30, 70, -10}, {0, -100, 0}, 10, 0.25f);
    system.update(1, translated);
    assert(system.impacts().size() == 1 && system.size() == 0);
    assert(near(system.impacts()[0].position, {30, 60, -10}));
    for (float dt : {0.0f, -1.0f, std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::infinity()})
    {
        spawn({-5, 0, 0}, {10, 0, 0});
        system.update(dt, sphere);
        assert(system.impacts().empty() && system.meteors()[0].lifetime == 10);
        system.clear();
    }
    for (auto invalid : {SphereCollider{{0, 0, 0}, 0}, SphereCollider{{0, 0, 0}, -1},
        SphereCollider{{std::numeric_limits<float>::quiet_NaN(), 0, 0}, 1}})
    {
        spawn({-5, 0, 0}, {10, 0, 0});
        bool rejected = false;
        try { system.update(1, invalid); }
        catch (const std::invalid_argument&) { rejected = true; }
        assert(rejected && system.size() == 1 && system.impacts().empty());
        system.clear();
    }
    std::cout << "Continuous collision, contact data and frame impact checks passed\n";
}
