#pragma once
#include <glm/glm.hpp>

// Runtime data only; size is the billboard diameter in world units.
struct Particle
{
    glm::vec3 position{0};
    glm::vec3 velocity{0};
    float size = 0.1f;
    float age = 0;
    float lifetime = 1;
};
