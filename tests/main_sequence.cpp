#include <cassert>
#include <cmath>
#include "cinematic/MainSequence.h"

int main()
{
    Scene scene;
    Timeline timeline;
    CinematicCamera camera;
    MeteorSystem system;
    MeteorShower shower(system);
    assert(!MainSequence::build(timeline, camera, scene, shower));
    SceneObject earth;
    earth.name = "Earth";
    earth.transform.position = glm::vec3(30, 50, 0);
    earth.transform.scale = glm::vec3(10);
    scene.addObject(earth);
    SceneObject clouds;
    clouds.name = "EarthClouds";
    scene.addObject(clouds);
    assert(MainSequence::build(timeline, camera, scene, shower));
    assert(timeline.isPlaying() && timeline.getDuration() == 30);
    assert(std::abs(glm::distance(camera.getPosition(), earth.transform.position) - 30) < 0.001f);
    timeline.update(10);
    assert(shower.isRunning());
    assert(std::abs(glm::distance(camera.getPosition(), earth.transform.position) - 40) < 0.001f);
    // Grow the scene after binding tracks: vector reallocations must not break them.
    for (int i = 0; i < 100; ++i) scene.addObject(SceneObject{});
    timeline.update(5);
    assert(std::abs(camera.getFOV() - 55) < 0.001f);
    assert(std::abs(scene.findObject("Earth")->transform.rotation.y - glm::radians(1500.0f)) < 0.001f);
    assert(scene.findObject("Earth")->transform.scale.x == 10);
    timeline.update(30);
    assert(!timeline.isPlaying() && timeline.getTime() == 30);
    assert(!shower.isRunning());
    assert(std::abs(glm::distance(camera.getPosition(), earth.transform.position) - 300) < 0.001f);
    assert(camera.getFOV() == 65);
    assert(std::abs(scene.findObject("EarthClouds")->transform.rotation.y - glm::radians(150.0f)) < 0.001f);
    MainSequence::reset(timeline, shower, system);
    assert(scene.findObject("Earth")->transform.rotation.y == 0);
    timeline.play();
    timeline.update(30);
    assert(timeline.getTime() == 30);
    assert(!shower.isRunning()); // One large step crosses start and stop in order.

    const SphereCollider collider{earth.transform.position, earth.transform.scale.x};
    auto step = [&]()
    {
        timeline.update(0.25f);
        system.update(0.25f, collider);
        shower.update(0.25f);
    };
    MainSequence::reset(timeline, shower, system);
    assert(timeline.getTime() == 0 && !timeline.isPlaying());
    assert(system.size() == 0 && !shower.isRunning());
    timeline.play();
    // Record the complete physical state at each fixed step, then replay it.
    std::vector<std::vector<Meteor>> states;
    std::vector<std::vector<MeteorImpact>> impacts;
    std::size_t impactCount = 0;
    for (int frame = 1; frame <= 120; ++frame)
    {
        step();
        states.push_back(system.meteors());
        impacts.push_back(system.impacts());
        impactCount += system.impacts().size();
        if (frame < 40) assert(system.size() == 0 && !shower.isRunning());
        if (frame == 40)
        {
            assert(shower.isRunning() && system.size() == 7);
            for (const auto& meteor : system.meteors())
                assert(meteor.lifetime >= 4 && meteor.lifetime <= 7); // No birth-frame aging.
        }
        if (frame == 41)
        {
            const auto& born = states[39][0];
            assert(glm::length(system.meteors()[0].transform.position
                - (born.transform.position + born.velocity * 0.25f)) < 0.0001f);
            assert(system.meteors()[0].lifetime == born.lifetime - 0.25f);
        }
        if (frame == 60) assert(shower.isRunning() && system.size() > 0);
        if (frame == 80) assert(!shower.isRunning() && system.size() > 0);
        if (frame == 81)
        {
            assert(!shower.isRunning() && system.size() <= states[79].size());
            assert(system.meteors().back().lifetime == states[79].back().lifetime - 0.25f);
        }
        if (frame >= 108) assert(system.size() == 0 && !shower.isRunning());
    }
    assert(impactCount > 0);
    for (int pass = 0; pass < 2; ++pass)
    {
        MainSequence::reset(timeline, shower, system);
        assert(system.size() == 0 && system.impacts().empty() && !shower.isRunning() && timeline.getTime() == 0);
        timeline.play();
        for (int frame = 0; frame < 120; ++frame)
        {
            step();
            assert(system.impacts().size() == impacts[frame].size());
            for (std::size_t i = 0; i < system.impacts().size(); ++i)
            {
                const auto& a = system.impacts()[i];
                const auto& b = impacts[frame][i];
                assert(a.position == b.position && a.normal == b.normal);
                assert(a.velocity == b.velocity && a.meteorScale == b.meteorScale);
            }
            assert(system.size() == states[frame].size());
            for (std::size_t i = 0; i < system.size(); ++i)
            {
                const auto& a = system.meteors()[i];
                const auto& b = states[frame][i];
                assert(a.transform.position == b.transform.position && a.velocity == b.velocity);
                assert(a.transform.scale == b.transform.scale && a.lifetime == b.lifetime);
            }
        }
        if (pass == 0)
        {
            MainSequence::reset(timeline, shower, system);
            timeline.play();
            for (int frame = 0; frame < 60; ++frame) step();
            assert(system.size() > 0); // Next pass also resets halfway through emission.
        }
    }
}
