#include "systems/EarthDamageSystem.h"
#include "graphics/Texture2D.h"
#include <glm/gtc/constants.hpp>
#include <algorithm>
#include <cmath>

namespace
{
    bool finite(const glm::vec3& v)
    {
        return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
    }
    glm::vec3 direction(const glm::vec2& uv)
    {
        const float theta = uv.x * glm::two_pi<float>();
        const float phi = (1 - uv.y) * glm::pi<float>();
        return {std::sin(phi) * std::cos(theta), std::cos(phi), std::sin(phi) * std::sin(theta)};
    }
}
EarthDamageSystem::EarthDamageSystem() : mPixels(Width * Height, 0)
{
    mDirections.reserve(mPixels.size());
    // Row zero is texture V=0 (south), without an image-file vertical flip.
    for (int y = 0; y < Height; ++y)
        for (int x = 0; x < Width; ++x)
            mDirections.push_back(direction({(x + 0.5f) / Width, (y + 0.5f) / Height}));
}
bool EarthDamageSystem::worldToUV(const glm::vec3& world, const Transform& earth, glm::vec2& uv)
{
    if (!finite(world) || !finite(earth.position) || !finite(earth.rotation) || !finite(earth.scale)
        || earth.scale.x <= 0 || earth.scale.y <= 0 || earth.scale.z <= 0) return false;
    const glm::dvec3 local(glm::inverse(glm::dmat4(earth.getMatrix())) * glm::dvec4(world, 1));
    const double length = glm::length(local);
    if (!std::isfinite(length) || length == 0) return false;
    const auto normal = local / length;
    double u = std::atan2(normal.z, normal.x) / glm::two_pi<double>();
    u -= std::floor(u);
    uv = {float(u), float(1 - std::acos(std::clamp(normal.y, -1.0, 1.0)) / glm::pi<double>())};
    return true;
}
void EarthDamageSystem::consume(const std::vector<MeteorImpact>& impacts, const Transform& earth, float localRadius)
{
    if (!std::isfinite(localRadius) || localRadius <= 0) return;
    const double worldRadius = double(localRadius) * std::max({earth.scale.x, earth.scale.y, earth.scale.z});
    for (const auto& impact : impacts)
    {
        glm::vec2 uv;
        if (!std::isfinite(impact.meteorScale) || impact.meteorScale <= 0
            || !worldToUV(impact.position, earth, uv)) continue;
        const auto center = direction(uv);
        const double radius = std::clamp(std::atan2(2.0 * impact.meteorScale, worldRadius), 0.025, 0.25);
        const float edge = float(std::cos(radius));
        // A spherical cap rather than a planar UV disk handles both seam and poles.
        for (std::size_t i = 0; i < mPixels.size(); ++i)
        {
            const float cosine = glm::dot(center, mDirections[i]);
            if (cosine <= edge) continue;
            const float t = std::clamp((cosine - edge) / (1 - edge), 0.0f, 1.0f);
            const float damage = 0.7f * t * t * (3 - 2 * t);
            const float value = std::min(1.0f, mPixels[i] + damage);
            if (value != mPixels[i]) { mPixels[i] = value; mDirty = true; }
        }
    }
}
void EarthDamageSystem::clear()
{
    std::fill(mPixels.begin(), mPixels.end(), 0);
    mDirty = true; // Reset must also replace the existing GPU contents.
}
bool EarthDamageSystem::upload(Texture2D& texture)
{
    if (!mDirty) return true;
    if (!texture.updateRed(Width, Height, mPixels.data())) return false;
    mDirty = false;
    return true;
}
