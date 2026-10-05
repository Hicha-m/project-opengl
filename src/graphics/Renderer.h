#pragma once

#include <glm/glm.hpp>

#include "scene/LightManager.h"
#include "scene/Scene.h"

// Draws objects and shared meshes using material settings and scene lights.
class Renderer
{
public:
    void render(Scene& scene, LightManager& lightManager, const glm::mat4& view, const glm::mat4& projection,
                const glm::vec3& cameraPosition);

    // Shared geometry/material with independent transforms, including populations.
    void renderMesh(Mesh& mesh, Material& material, const Transform& transform, LightManager& lightManager,
                    const glm::mat4& view, const glm::mat4& projection, const glm::vec3& cameraPosition);

private:
    void renderObject(SceneObject& object, LightManager& lightManager, const glm::mat4& view,
                      const glm::mat4& projection, const glm::vec3& cameraPosition);
};
