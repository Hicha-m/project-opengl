#pragma once
#include <memory>
#include <vector>
#include "systems/Meteor.h"
#include "systems/MeteorImpact.h"
#include "geometry/SphereCollider.h"

class Renderer;
class LightManager;
struct MeteorResources;

class MeteorSystem
{
public:
    MeteorSystem();
    ~MeteorSystem();
    MeteorSystem(const MeteorSystem&) = delete;
    MeteorSystem& operator=(const MeteorSystem&) = delete;

    // CPU simulation is usable without OpenGL. Invalid input is rejected.
    bool spawn(const Transform& transform, const glm::vec3& velocity, float lifetime);
    void update(float deltaTime);
    void update(float deltaTime, const SphereCollider& collider);
    // Cleared on every update (even invalid dt) and clear; no effects are triggered.
    const std::vector<MeteorImpact>& impacts() const { return mImpacts; }
    void clear(); // Removes instances, keeps shared graphics available for reuse.
    std::size_t size() const { return mMeteors.size(); }
    // Read-only observation for diagnostics; invalidated by spawn/update/clear.
    const std::vector<Meteor>& meteors() const { return mMeteors; }

    // Idempotent; creation and release must run with the owning GL context current.
    bool initGraphics();
    void releaseGraphics();
    bool graphicsReady() const { return bool(mResources); }
    void render(Renderer& renderer, LightManager& lights,
        const glm::mat4& view, const glm::mat4& projection,
        const glm::vec3& cameraPosition);

private:
    void simulate(float deltaTime, const SphereCollider* collider);
    std::vector<Meteor> mMeteors;
    std::vector<MeteorImpact> mImpacts;
    std::unique_ptr<MeteorResources> mResources;
};
