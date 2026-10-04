#pragma once
#include "geometry/Sphere.h"
#include "scene/Material.h"

// One resource set per system. Requires a current GL context throughout its life.
struct MeteorResources
{
    ShaderProgram shader;
    Texture2D texture;
    Sphere sphere{1.0f, 12, 8};
    Material material{&shader};

    MeteorResources() = default;
    MeteorResources(const MeteorResources&) = delete;
    MeteorResources& operator=(const MeteorResources&) = delete;
    bool load();
};
