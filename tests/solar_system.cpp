#include "systems/SolarSystem.h"
#include <cassert>
#include <cmath>
#include <iostream>
int main()
{
    Scene scene;
    for (const auto& p : SolarSystem::Planets)
    {
        SceneObject body;
        body.name = p.name;
        body.transform.scale = glm::vec3(p.size);
        scene.addObject(body);
    }
    for (const char* name : {"Moon", "EarthClouds", "SaturnRings", "OrbitEarth", "OrbitMoon"})
    {
        SceneObject body;
        body.name = name;
        scene.addObject(body);
    }
    SolarSystem solar;
    solar.reset(scene);
    assert(scene.findObject("Earth")->transform.position == glm::vec3(30, 50, 0));
    float radius = 0, speed = 100;
    for (const auto& p : SolarSystem::Planets)
    {
        assert(p.radius > radius && p.angularSpeed < speed);
        radius = p.radius;
        speed = p.angularSpeed;
        assert(std::abs(glm::distance(SolarSystem::planetPosition(p, 37), SolarSystem::SunCenter) -
                        p.radius) < 0.001f);
    }
    glm::vec3 replayMoon, replayVelocity, replayEarth;
    for (int pass = 0; pass < 2; ++pass)
    {
        solar.reset(scene);
        assert(!solar.moonReleased() && scene.findObject("OrbitMoon")->visible);
        for (int i = 1; i <= 200; ++i)
        {
            solar.update(scene, i * 0.25f, 0.25f, false);
        }
        const auto earth = scene.findObject("Earth")->transform.position;
        const auto moon = scene.findObject("Moon")->transform.position;
        const auto jupiter = scene.findObject("Jupiter")->transform.position;
        assert(glm::distance(moon, earth) > 27 && glm::distance(moon, earth) < 29);
        solar.update(scene, 50.25f, 0.25f, true);
        assert(solar.moonReleased() && !scene.findObject("OrbitMoon")->visible &&
               !scene.findObject("OrbitEarth")->visible);
        const auto velocity = solar.moonVelocity();
        assert(glm::length(scene.findObject("Moon")->transform.position - (moon + velocity * 0.25f)) <
               0.00001f);
        for (int i = 202; i <= 400; ++i)
        {
            solar.update(scene, i * 0.25f, 0.25f, true);
        }
        assert(scene.findObject("Earth")->transform.position == earth);
        assert(scene.findObject("Jupiter")->transform.position != jupiter);
        assert(glm::distance(scene.findObject("Moon")->transform.position, earth) > 70);
        if (!pass)
        {
            replayMoon = scene.findObject("Moon")->transform.position;
            replayVelocity = velocity;
            replayEarth = earth;
        }
        else
        {
            assert(scene.findObject("Moon")->transform.position == replayMoon && velocity == replayVelocity &&
                   earth == replayEarth);
        }
    }
    solar.reset(scene);
    assert(scene.findObject("OrbitMoon")->visible && scene.findObject("OrbitEarth")->visible);
    std::cout
        << "Compressed solar orbits, lunar tangent drift, captured Earth and deterministic reset passed\n";
}
