#pragma once

#include <string>
#include <vector>

#include "scene/SceneObject.h"

// Named renderable objects; graphics resources are owned separately.
class Scene
{
public:
    std::vector<SceneObject> objects;

    void addObject(const SceneObject& object) { objects.push_back(object); }

    SceneObject* findObject(const std::string& name)
    {
        for (SceneObject& object : objects)
        {
            if (object.name == name)
            {
                return &object;
            }
        }

        return nullptr;
    }

    std::size_t getObjectCount() const { return objects.size(); }

    const SceneObject* findObject(const std::string& name) const
    {
        for (const auto& object : objects)
        {
            if (object.name == name)
            {
                return &object;
            }
        }
        return nullptr;
    }
};
