#pragma once
#include "scene/Scene.h"
#include "scene/SceneResources.h"
#include "scene/LightManager.h"
#include "geometry/SphereCollider.h"

namespace SceneSetup
{
    bool build(Scene& scene, LightManager& lightManager, SceneResources& resources);
    void update(Scene& scene, const glm::vec3& cameraPosition);
    // Throws if Earth is missing or its scale is not positive, finite and uniform.
    SphereCollider earthCollider(const Scene& scene, const SceneResources& resources);
}
