#pragma once
#include <vector>
#include "systems/MeteorImpact.h"
#include "scene/Transform.h"
class Texture2D;

// Persistent CPU mask in the same local spherical UVs as geometry/Sphere.
// No references to the meteor, light or particle simulation systems.
class EarthDamageSystem
{
public:
    static constexpr int Width = 512, Height = 256;
    EarthDamageSystem();
    static bool worldToUV(const glm::vec3& worldPosition, const Transform& earth, glm::vec2& uv);
    void consume(const std::vector<MeteorImpact>& impacts, const Transform& earth, float localRadius = 1.0f);
    void update(float deltaTime); // Cool existing heat before consuming new impacts.
    void clear();
    const std::vector<float>& pixels() const { return mPixels; }
    const std::vector<float>& heatPixels() const { return mHeat; }
    bool heatDirty() const { return mHeatDirty; }
    static constexpr float CoolingTime = 1.5f;
    static constexpr float MaxHeat = 6.0f;
    bool dirty() const { return mDirty; }
    // Upload only when dirty. Destination is owned by SceneResources.
    bool upload(Texture2D& texture);
    bool uploadHeat(Texture2D& texture);
    bool upload(Texture2D& damage, Texture2D& heat);
private:
    std::vector<float> mPixels;
    std::vector<float> mHeat;
    bool mHeatDirty = true;
    std::vector<glm::vec3> mDirections;
    bool mDirty = true;
};
