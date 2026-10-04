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
    // Temporary heat shares local UVs and persists independently of burn damage.
    earth = Transform{}; earth.scale = glm::vec3(10);
    impact = {{0,0,10},{0,0,1},{0,0,-10},0.5f};
    damage.consume({impact},earth);
    const auto permanent = damage.pixels();
    const auto hot = damage.heatPixels();
    const float hottest = *std::max_element(hot.begin(),hot.end());
    assert(hottest > 1.9f);
    damage.update(EarthDamageSystem::CoolingTime);
    for (std::size_t i = 0; i < hot.size(); ++i) {
        assert(damage.heatPixels()[i] <= hot[i]);
        if (hot[i] > 0.01f) assert(std::abs(damage.heatPixels()[i] - hot[i]/std::exp(1.0f)) < 0.00001f);
    }
    assert(damage.pixels() == permanent);
    const auto cooled = damage.heatPixels();
    for (float dt : {0.0f,-1.0f,std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity()})
        damage.update(dt);
    assert(damage.heatPixels() == cooled);
    damage.update(30);
    for (float heat : damage.heatPixels()) assert(heat == 0);
    assert(damage.pixels() == permanent);
    // Nearby impacts add heat; the saturation limit keeps it finite.
    damage.clear(); damage.consume({impact},earth);
    const auto single = damage.heatPixels();
    damage.consume({impact},earth);
    for (std::size_t i = 0; i < single.size(); ++i) assert(damage.heatPixels()[i] == single[i]*2);
    for (int repeat = 0; repeat < 8; ++repeat) damage.consume({impact},earth);
    assert(*std::max_element(damage.heatPixels().begin(),damage.heatPixels().end()) == EarthDamageSystem::MaxHeat);
    const auto overlap = damage.heatPixels();
    damage.clear(); damage.consume(std::vector<MeteorImpact>(10,impact),earth);
    assert(damage.heatPixels() == overlap);
    // Cooling is insensitive to cadence, within float arithmetic tolerance.
    damage.clear(); damage.consume({impact},earth); damage.update(1);
    const auto oneSecond = damage.heatPixels();
    for (int fps : {30,60,144}) {
        damage.clear(); damage.consume({impact},earth);
        for (int frame = 0; frame < fps; ++frame) damage.update(1.0f/fps);
        for (std::size_t i = 0; i < single.size(); ++i)
            assert(std::abs(damage.heatPixels()[i]-oneSecond[i]) < 0.00005f);
    }
    const auto replay = damage.heatPixels();
    damage.clear(); damage.consume({impact},earth);
    for (int frame = 0; frame < 144; ++frame) damage.update(1.0f/144);
    assert(damage.heatPixels() == replay);
    // Rotation maps back to the same local heat footprint.
    const auto localHeat = damage.heatPixels();
    earth.rotation.y = 0.7f; impact.position = world(earth,{0,0,1});
    damage.clear(); damage.consume({impact},earth);
    for (int frame = 0; frame < 144; ++frame) damage.update(1.0f/144);
    for (std::size_t i = 0; i < localHeat.size(); ++i)
        assert(std::abs(damage.heatPixels()[i]-localHeat[i]) < 0.0001f);
    damage.clear(); earth = Transform{};
    impact.position = {1,0,0}; damage.consume({impact},earth);
    assert(damage.heatPixels()[row*EarthDamageSystem::Width] > 0);
    assert(std::abs(damage.heatPixels()[row*EarthDamageSystem::Width] - damage.heatPixels()[row*EarthDamageSystem::Width+EarthDamageSystem::Width-1]) < 0.0001f);
    damage.clear();
    for (float heat : damage.heatPixels()) assert(heat == 0);
    assert(damage.heatDirty());
    std::cout << "Earth burn and heat mapping, cooling, accumulation, seam, rotation and replay checks passed\n";
}
