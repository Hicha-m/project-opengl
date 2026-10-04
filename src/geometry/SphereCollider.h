#pragma once
#include <cmath>
#include <glm/glm.hpp>

// World-space sphere, independent of rendering and scene objects.
struct SphereCollider
{
    glm::vec3 center{0.0f};
    float radius = 1.0f;

    bool isValid() const
    {
        return std::isfinite(center.x) && std::isfinite(center.y)
            && std::isfinite(center.z) && std::isfinite(radius) && radius > 0;
    }
};
