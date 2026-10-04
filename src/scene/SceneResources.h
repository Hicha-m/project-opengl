#pragma once
#include "ShaderProgram.h"
#include "Texture2D.h"
#include "../Sphere.h"

// Construct only with a current OpenGL context. Destroy before closing it.
// Scene materials and meshes borrow these resources; do not copy or move them.
struct SceneResources
{
    ShaderProgram earthShader, cloudShader, sunShader, starShader;
    Texture2D earthDayTexture, earthNightTexture, earthSpecularTexture;
    Texture2D earthNormalTexture, earthCloudsTexture, sunTexture, starTexture;
    Sphere earthSphere{1.0f, 32, 32};
    Sphere sunSphere{1.0f, 32, 32};
    Sphere starSphere{1.0f, 32, 32};

    SceneResources() = default;
    SceneResources(const SceneResources&) = delete;
    SceneResources& operator=(const SceneResources&) = delete;
};
