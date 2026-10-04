#pragma once

#include <vector>
#include <string>

#include "scene/SceneObject.h"

class Scene
{
public:

    std::vector<SceneObject> objects;

    void addObject(const SceneObject& object)
    {
        objects.push_back(object);
    }

    SceneObject* getObject(unsigned int index)
    {
        if (index >= objects.size())
            return nullptr;

        return &objects[index];
    }

    SceneObject* findObject(const std::string& name)
    {
        for (SceneObject& object : objects)
        {
            if (object.name == name)
                return &object;
        }

        return nullptr;
    }

    size_t getObjectCount() const
    {
        return objects.size();
    }

    const SceneObject* findObject(const std::string& name) const
    {
        for (const auto& object : objects)
            if (object.name == name) return &object;
        return nullptr;
    }
};
