#pragma once
#include <glm/glm.hpp>

// Contact data only, ready for later light/particle/damage consumers.
struct MeteorImpact
{
    glm::vec3 position{0.0f}; // On the target surface, not at the meteor center.
    glm::vec3 normal{0.0f};   // Outward unit surface normal.
    glm::vec3 velocity{0.0f};
    float meteorScale = 0.0f; // Maximum scale component; unit-radius meteor mesh.
};
