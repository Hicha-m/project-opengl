#pragma once
#include "scene/Transform.h"
#include <cstdint>

using MeteorId = std::uint64_t;

// Only live meteors are stored: no GPU resources or SceneObject per instance.
struct Meteor
{
    Transform transform;
    glm::vec3 velocity{0.0f};
    float lifetime = 0.0f; // Remaining seconds.
    MeteorId id = 0;       // Stable until removal; zero is not assigned by MeteorSystem.
};
