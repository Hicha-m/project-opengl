#pragma once

#include <string>

#include "graphics/Mesh.h"
#include "scene/Material.h"
#include "scene/Transform.h"

// One visible object: name, transform, borrowed mesh and material.
class SceneObject
{
public:
    std::string name;
    bool visible = true;
    Transform transform;
    Mesh* mesh = nullptr;
    Material material;
    SceneObject() = default;
    SceneObject(const std::string& name, Mesh* mesh, ShaderProgram* shader)
        : name(name), mesh(mesh), material(shader)
    {
    }

    bool isValid() const { return mesh != nullptr && material.shader != nullptr; }
};
