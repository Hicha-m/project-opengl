#pragma once
#include "scene/Scene.h"
#include "scene/SceneResources.h"
#include "scene/LightManager.h"

namespace SceneSetup
{
    bool build(Scene& scene, LightManager& lightManager, SceneResources& resources);
    void update(Scene& scene, const glm::vec3& cameraPosition);
}
