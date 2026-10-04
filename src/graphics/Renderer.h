#pragma once

#include <glm/glm.hpp>

#include "scene/Scene.h"
#include "scene/LightManager.h"


class Renderer
{
public:

    void render(
        Scene& scene,
        LightManager& lightManager,
        const glm::mat4& view,
        const glm::mat4& projection,
        const glm::vec3& cameraPosition
    );


private:

    void renderObject(
        SceneObject& object,
        LightManager& lightManager,
        const glm::mat4& view,
        const glm::mat4& projection,
        const glm::vec3& cameraPosition
    );
};