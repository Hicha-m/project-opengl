#pragma once
#include "Scene.h"
#include "SceneResources.h"
#include "LightManager.h"

namespace SceneSetup
{
    bool build(Scene& scene, LightManager& lightManager, SceneResources& resources);
    void update(Scene& scene, const glm::vec3& cameraPosition);
}
