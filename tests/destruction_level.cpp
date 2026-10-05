#include "systems/EarthDamageSystem.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>

int main()
{
    EarthDamageSystem damage;
    Transform earth;
    MeteorImpact small{{0, 0, 1}, {0, 0, 1}, {0, 0, -4}, 0.25f};
    assert(damage.destructionLevel() == 0);
    damage.consume({small}, earth);
    const float smallLevel = damage.destructionLevel();
    assert(smallLevel > 0 && smallLevel < 1);
    auto larger = small;
    larger.meteorScale *= 2;
    damage.clear();
    damage.consume({larger}, earth);
    assert(std::abs(damage.destructionLevel() / smallLevel - 8) < 0.00001f);
    auto faster = small;
    faster.velocity *= 2;
    damage.clear();
    damage.consume({faster}, earth);
    assert(std::abs(damage.destructionLevel() / smallLevel - 4) < 0.00001f);
    faster.velocity *= -1; // Direction must not change the global contribution.
    damage.clear();
    damage.consume({faster}, earth);
    const float fasterLevel = damage.destructionLevel();
    assert(std::abs(fasterLevel / smallLevel - 4) < 0.00001f);
    damage.update(100);
    assert(damage.destructionLevel() == fasterLevel);
    damage.consume({}, earth);
    assert(damage.destructionLevel() == fasterLevel);

    // Repeated strikes keep progressing even on an already saturated burn patch.
    damage.clear();
    MeteorImpact medium{
        {0, 0, 1}, {0, 0, 1}, {0, 0, -float(std::sqrt(EarthDamageSystem::DestructionEnergyBudget * 0.1))}, 1};
    for (int hit = 0; hit < 12; ++hit)
    {
        const float before = damage.destructionLevel();
        damage.consume({medium}, earth);
        const float after = damage.destructionLevel();
        assert(after >= before && after >= 0 && after <= 1);
        if (hit < 9)
        {
            assert(after > before);
        }
        if (hit >= 1)
        {
            assert(*std::max_element(damage.pixels().begin(), damage.pixels().end()) == 1);
        }
    }
    assert(damage.destructionLevel() == 1);
    damage.update(100);
    assert(damage.destructionLevel() == 1);
    damage.clear();
    assert(damage.destructionLevel() == 0);
    auto stationary = small;
    stationary.velocity = {0, 0, 0};
    damage.consume({stationary}, earth);
    assert(damage.destructionLevel() == 0);

    // Invalid events/transforms cannot corrupt or increase the global state.
    damage.consume({small}, earth);
    const float beforeInvalid = damage.destructionLevel();
    auto invalid = small;
    invalid.velocity.x = std::numeric_limits<float>::quiet_NaN();
    damage.consume({invalid}, earth);
    invalid = small;
    invalid.velocity.y = std::numeric_limits<float>::infinity();
    damage.consume({invalid}, earth);
    invalid = small;
    invalid.meteorScale = -1;
    damage.consume({invalid}, earth);
    invalid = small;
    invalid.meteorScale = std::numeric_limits<float>::infinity();
    damage.consume({invalid}, earth);
    invalid = small;
    invalid.position = {0, 0, 0};
    damage.consume({invalid}, earth);
    damage.consume({small}, earth, 0);
    earth.scale.x = 0;
    damage.consume({small}, earth);
    earth = Transform{};
    assert(damage.destructionLevel() == beforeInvalid);
    // Very large finite inputs saturate safely using double intermediates.
    auto enormous = small;
    enormous.meteorScale = std::numeric_limits<float>::max();
    enormous.velocity = glm::vec3(std::numeric_limits<float>::max());
    damage.consume({enormous}, earth);
    assert(damage.destructionLevel() == 1);

    // Batch boundaries, timing, translation and rotation do not affect the energy.
    const std::vector<MeteorImpact> events{small, larger, faster, small, medium};
    damage.clear();
    damage.consume(events, earth);
    const float batchLevel = damage.destructionLevel();
    std::vector<float> firstPass;
    for (int pass = 0; pass < 2; ++pass)
    {
        damage.clear();
        assert(damage.destructionLevel() == 0);
        for (std::size_t i = 0; i < events.size(); ++i)
        {
            damage.update(float(i + 1) * 0.1f);
            damage.consume({events[i]}, earth);
            if (!pass)
            {
                firstPass.push_back(damage.destructionLevel());
            }
            else
            {
                assert(damage.destructionLevel() == firstPass[i]);
            }
        }
        assert(damage.destructionLevel() == batchLevel);
    }
    earth.position = {30, 50, 0};
    earth.rotation = {0.4f, 0.9f, -0.2f};
    earth.scale = glm::vec3(10);
    auto transformed = events;
    for (auto& event : transformed)
    {
        event.position = glm::vec3(earth.getMatrix() * glm::vec4(event.position, 1));
    }
    damage.clear();
    damage.consume(transformed, earth);
    assert(damage.destructionLevel() == batchLevel);
    std::cout << "Global destruction energy, monotonicity, bounds, invalid inputs and replay checks passed\n";
}
