#pragma once
#include <memory>
#include <vector>
#include "scene/Transform.h"
#include "scene/LightManager.h"
class Mesh;
class Renderer;
class Material;
struct EarthBreakupResources;

struct EarthFragment
{
    glm::vec3 pivot{0}; // Fixed local geometry anchor.
    Transform transform;
    glm::vec3 velocity{0}, angularVelocity{0};
};

// Prepared crust pieces, deterministic cinematic motion. No meteor dependency.
class EarthBreakupSystem
{
public:
    static constexpr unsigned Sectors = 8, Bands = 4, FragmentCount = Sectors * Bands;
    static constexpr float Threshold = 0.95f;
    EarthBreakupSystem();
    ~EarthBreakupSystem();
    EarthBreakupSystem(const EarthBreakupSystem&) = delete;
    EarthBreakupSystem& operator=(const EarthBreakupSystem&) = delete;
    void update(float dt, float destructionLevel, const Transform& earth);
    void reset();
    bool active() const { return mActive; }
    const std::vector<EarthFragment>& fragments() const { return mFragments; }
    PointLight coreLight() const;
    // Called after flashes have replaced the transient light list for this frame.
    void publish(LightManager& lights) const;
    bool initGraphics(const Mesh& earthMesh);
    void releaseGraphics();
    bool graphicsReady() const { return bool(mResources); }
    void render(Renderer& renderer, Material& crust, LightManager& lights,
        const glm::mat4& view, const glm::mat4& projection, const glm::vec3& eye);
private:
    bool mActive = false;
    Transform mEarth;
    std::vector<EarthFragment> mFragments;
    std::unique_ptr<EarthBreakupResources> mResources;
};
