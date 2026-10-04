#pragma once

#include <string>

#include "scene/Transform.h"
#include "scene/Material.h"
#include "graphics/Mesh.h"

class SceneObject
{
public:

    std::string name;
    bool visible = true;

    Transform transform;

    Mesh* mesh = nullptr;

    Material material;


    SceneObject() = default;


    SceneObject(
        const std::string& name,
        Mesh* mesh,
        ShaderProgram* shader
    )
        : name(name),
          mesh(mesh),
          material(shader)
    {
    }


    bool isValid() const
    {
        return mesh != nullptr &&
               material.shader != nullptr;
    }
};