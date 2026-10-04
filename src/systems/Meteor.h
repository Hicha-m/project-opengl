#pragma once
#include "scene/Transform.h"

// Only live meteors are stored: no GPU resources or SceneObject per instance.
struct Meteor
{
    Transform transform;
    glm::vec3 velocity{0.0f};
    float lifetime = 0.0f; // Remaining seconds.
};
