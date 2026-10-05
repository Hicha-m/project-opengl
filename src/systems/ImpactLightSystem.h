#pragma once
#include "systems/ImpactLight.h"
#include "systems/MeteorImpact.h"
#include <vector>

class LightManager;

// CPU-only consumer; knows neither MeteorSystem nor the impacted scene object.
class ImpactLightSystem
{
public:
    static constexpr float SurfaceOffset = 0.2f;
    void consume(const std::vector<MeteorImpact>& impacts);
    void update(float deltaTime);
    void clear();
    const std::vector<ImpactLight>& lights() const { return mLights; }
    // Replaces only transient lights; persistent scene lights remain intact.
    void publish(LightManager& manager) const;

private:
    std::vector<ImpactLight> mLights;
};
