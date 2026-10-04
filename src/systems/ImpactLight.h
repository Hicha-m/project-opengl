#pragma once
#include <glm/glm.hpp>

struct ImpactLight
{
    glm::vec3 position{0.0f};
    glm::vec3 color{1.0f, 0.45f, 0.12f};
    float initialIntensity = 0.0f;
    float intensity = 0.0f;
    float age = 0.0f;
    float lifetime = 0.5f;
    // consume -> update -> render: skip aging on the birth frame.
    bool fresh = true;
};
