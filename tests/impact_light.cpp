#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include "systems/ImpactLightSystem.h"
#include "scene/LightManager.h"

int main()
{
    ImpactLightSystem system;
    LightManager manager;
    PointLight permanent;
    permanent.position = {100, 0, 0};
    permanent.intensity = 4;
    manager.addPointLight(permanent);
    const MeteorImpact impact{{10, 20, 30}, {0, 1, 0}, {0, -10, 0}, 0.5f};
    system.consume({impact});
    assert(system.lights().size() == 1);
    assert(glm::distance(system.lights()[0].position,
        impact.position + impact.normal * ImpactLightSystem::SurfaceOffset) < 0.00001f);
    const float initial = system.lights()[0].initialIntensity;
    assert(initial == 15);
    system.update(10); // Even a long birth frame must show the initial flash.
    assert(system.lights().size() == 1 && system.lights()[0].age == 0);
    assert(system.lights()[0].intensity == initial);
    system.publish(manager);
    assert(manager.shaderPointLights().size() == 2);
    assert(manager.shaderPointLights()[0].intensity == initial);
    assert(manager.getPointLights().size() == 1);
    system.update(0.25f);
    assert(system.lights()[0].intensity == initial * 0.25f);
    system.consume({impact});
    system.update(0.125f);
    assert(system.lights()[0].intensity < initial * 0.25f);
    assert(system.lights()[1].intensity == initial && system.lights()[1].age == 0);
    const auto beforeInvalid = system.lights();
    for (float dt : {-1.0f, std::numeric_limits<float>::infinity(),
        std::numeric_limits<float>::quiet_NaN()}) system.update(dt);
    assert(system.lights()[0].age == beforeInvalid[0].age);
    system.update(0.5f);
    assert(system.lights().empty());
    system.publish(manager);
    assert(manager.shaderPointLights().size() == 1);

    MeteorImpact small = impact, large = impact;
    small.velocity *= 0.1f;
    small.meteorScale = 0.1f;
    large.velocity *= 100;
    large.meteorScale = 100;
    system.consume({small, impact, large});
    assert(system.lights()[0].intensity == 2);
    assert(system.lights()[1].intensity == initial);
    assert(system.lights()[2].intensity == 30);
    auto invalid = impact;
    invalid.normal = glm::vec3(0);
    system.consume({invalid});
    invalid = impact;
    invalid.meteorScale = -1;
    system.consume({invalid});
    invalid = impact;
    invalid.velocity.x = std::numeric_limits<float>::quiet_NaN();
    system.consume({invalid});
    assert(system.lights().size() == 3);

    std::vector<MeteorImpact> many;
    for (int i = 0; i < 80; ++i)
    {
        auto item = impact;
        item.position.x = float(i);
        // Two equal lights per strength, to exercise deterministic ties.
        item.velocity = {0, -float(1 + i / 2), 0};
        item.meteorScale = 0.1f;
        many.push_back(item);
    }
    std::vector<ImpactLight> replay;
    std::vector<PointLight> selectedReplay;
    for (int pass = 0; pass < 2; ++pass)
    {
        system.clear();
        system.publish(manager);
        assert(system.lights().empty() && manager.shaderPointLights().size() == 1);
        system.consume(many);
        system.update(1);
        assert(system.lights().size() == 80);
        system.publish(manager);
        auto selected = manager.shaderPointLights();
        assert(selected.size() == LightManager::MaxPointLights);
        assert(selected.front().position.x == 78 && selected[1].position.x == 79);
        for (std::size_t i = 1; i < selected.size(); ++i)
            assert(selected[i - 1].intensity >= selected[i].intensity);
        system.update(0.125f);
        if (!pass)
        {
            replay = system.lights();
            selectedReplay = selected;
        }
        else for (std::size_t i = 0; i < replay.size(); ++i)
        {
            assert(system.lights()[i].position == replay[i].position);
            assert(system.lights()[i].intensity == replay[i].intensity);
        }
        if (pass) for (std::size_t i = 0; i < selected.size(); ++i)
        {
            assert(selected[i].position == selectedReplay[i].position);
            assert(selected[i].intensity == selectedReplay[i].intensity);
        }
    }
    // GPU limit also applies when only persistent lights overflow.
    system.clear();
    system.publish(manager);
    for (int i = 0; i < 100; ++i) manager.addPointLight(permanent);
    assert(manager.shaderPointLights().size() == LightManager::MaxPointLights);
    std::cout << "Impact flash birth, decay, expiry, selection and replay checks passed\n";
}
