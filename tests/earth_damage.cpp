#include <cassert>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include "systems/EarthDamageSystem.h"

static glm::vec3 world(const Transform& transform, const glm::vec3& local)
{
    return glm::vec3(transform.getMatrix() * glm::vec4(local,1));
}
static float total(const EarthDamageSystem& damage)
{
    float sum = 0; for (float value : damage.pixels()) sum += value; return sum;
}
int main()
{
    EarthDamageSystem damage;
    assert(total(damage) == 0);
    Transform earth;
    glm::vec2 uv;
    for (const auto& pair : std::vector<std::pair<glm::vec3,glm::vec2>>{
        {{1,0,0},{0,0.5f}}, {{0,0,1},{0.25f,0.5f}}, {{-1,0,0},{0.5f,0.5f}},
        {{0,0,-1},{0.75f,0.5f}}, {{0,1,0},{0,1}}, {{0,-1,0},{0,0}}}) {
        assert(EarthDamageSystem::worldToUV(pair.first,earth,uv));
        assert(glm::distance(uv,pair.second) < 0.00001f);
    }
    earth.position = {30,50,-4}; earth.scale = glm::vec3(10);
    earth.rotation = {0.3f,1.1f,-0.4f};
    assert(EarthDamageSystem::worldToUV(world(earth,{0,0,1}),earth,uv));
    assert(glm::distance(uv,{0.25f,0.5f}) < 0.00001f);
    // The same local contact after rotation produces the same attached footprint.
    MeteorImpact impact{world(earth,{0,0,1}),{0,0,1},{0,0,-10},0.5f};
    damage.consume({impact},earth);
    const auto first = damage.pixels(); assert(total(damage) > 0);
    earth.rotation = {-0.7f,-1.3f,0.2f};
    impact.position = world(earth,{0,0,1});
    EarthDamageSystem rotated; rotated.consume({impact},earth);
    for (std::size_t i = 0; i < first.size(); ++i) assert(std::abs(first[i]-rotated.pixels()[i]) < 0.0001f);
    const float firstTotal = total(damage);
    damage.consume({impact},earth); assert(total(damage) > firstTotal);
    for (float value : damage.pixels()) assert(value >= 0 && value <= 1);
    for (int repeat = 0; repeat < 5; ++repeat) damage.consume({impact},earth);
    assert(*std::max_element(damage.pixels().begin(),damage.pixels().end()) == 1);
    impact.position = world(earth,{0,0,-1});
    const auto prior = damage.pixels(); damage.consume({impact},earth);
    for (std::size_t i = 0; i < prior.size(); ++i) assert(damage.pixels()[i] >= prior[i]);

    // A mark at U=0 crosses into both edges of the texture, symmetrically.
    earth = Transform{}; damage.clear();
    impact.position = {1,0,0}; impact.meteorScale = 0.1f;
    damage.consume({impact},earth);
    const int row = EarthDamageSystem::Height/2;
    const auto& map = damage.pixels();
    assert(map[row*EarthDamageSystem::Width] > 0.6f);
    assert(map[row*EarthDamageSystem::Width + EarthDamageSystem::Width-1] > 0.6f);
    assert(std::abs(map[row*EarthDamageSystem::Width]-map[row*EarthDamageSystem::Width+EarthDamageSystem::Width-1]) < 0.0001f);
    assert(map[row*EarthDamageSystem::Width + EarthDamageSystem::Width/2] == 0);
    const auto seam = map;
    damage.clear(); damage.consume({impact},earth); assert(damage.pixels() == seam);
    // Circular caps include every longitude at the pole without wrapping V.
    damage.clear(); impact.position = {0,1,0}; damage.consume({impact},earth);
    for (int x = 0; x < EarthDamageSystem::Width; ++x) {
        assert(damage.pixels()[(EarthDamageSystem::Height-1)*EarthDamageSystem::Width+x] > 0.6f);
        assert(damage.pixels()[x] == 0);
    }
    const auto valid = damage.pixels();
    impact.position = {0,0,0}; damage.consume({impact},earth); assert(damage.pixels() == valid);
    impact.position.x = std::numeric_limits<float>::quiet_NaN(); damage.consume({impact},earth);
    assert(damage.pixels() == valid);
    earth.scale.x = 0; assert(!EarthDamageSystem::worldToUV({1,0,0},earth,uv));
    damage.clear(); assert(total(damage) == 0 && damage.dirty());
    std::cout << "Earth damage local UVs, rotation, accumulation, seam, poles and reset checks passed\n";
}
