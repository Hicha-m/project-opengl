#pragma once
#include <array>
#include "scene/Scene.h"

struct PlanetOrbit {
    const char* name;
    float radius, size, angularSpeed, phase, tilt;
};

// Compressed cinematic orbits, no gravitational solver or GPU ownership.
class SolarSystem {
public:
    static const std::array<PlanetOrbit,8> Planets;
    static const glm::vec3 SunCenter;
    static constexpr float MoonRadius=28.0f, MoonSpeed=0.08f;
    static glm::vec3 planetPosition(const PlanetOrbit& planet, float time);
    static glm::vec3 planetVelocity(const PlanetOrbit& planet, float time);
    void update(Scene& scene, float time, float dt, bool earthDestroyed);
    void reset(Scene& scene);
    bool moonReleased() const { return mMoonReleased; }
    glm::vec3 moonVelocity() const { return mMoonVelocity; }
private:
    bool mMoonReleased=false;
    glm::vec3 mMoonVelocity{0}, mMoonPosition{0};
};
