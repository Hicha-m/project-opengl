#pragma once

#include <string>

#include "Transform.h"
#include "Material.h"
#include "Mesh.h"

class SceneObject
{
public:

    std::string name;

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