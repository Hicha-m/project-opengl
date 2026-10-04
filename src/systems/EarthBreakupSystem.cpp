#include "systems/EarthBreakupSystem.h"
#include "graphics/Renderer.h"
#include "geometry/Sphere.h"
#include <glm/gtc/constants.hpp>
#include <algorithm>
#include <cmath>
#include <random>

namespace {
    glm::vec3 sphere(float u, float v) {
        const float phi = v * glm::pi<float>(), theta = u * glm::two_pi<float>();
        return {std::sin(phi)*std::cos(theta),std::cos(phi),std::sin(phi)*std::sin(theta)};
    }
    bool load(ShaderProgram& shader, const char* vertex, const char* fragment) {
        if (!shader.loadShaders(vertex,fragment)) return false;
        GLint linked; glGetProgramiv(shader.getProgram(),GL_LINK_STATUS,&linked);
        return linked == GL_TRUE;
    }
    bool finite(const glm::vec3& v) {
        return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
    }
}
struct EarthBreakupResources
{
    std::vector<std::unique_ptr<Mesh>> outer, inner;
    ShaderProgram coreShader, interiorShader;
    Sphere core{0.55f,32,32};
    Material coreMaterial, interiorMaterial;
};
EarthBreakupSystem::EarthBreakupSystem()
{
    for (unsigned b = 0; b < Bands; ++b)
        for (unsigned s = 0; s < Sectors; ++s) {
            EarthFragment fragment;
            fragment.pivot = sphere((s+0.5f)/Sectors,(b+0.5f)/Bands)*0.85f;
            mFragments.push_back(fragment);
        }
}
EarthBreakupSystem::~EarthBreakupSystem() = default;
void EarthBreakupSystem::reset()
{
    mActive = false; mEarth = Transform{};
    for (auto& f : mFragments) { f.transform = Transform{}; f.velocity = f.angularVelocity = glm::vec3(0); }
}
void EarthBreakupSystem::update(float dt, float level, const Transform& earth)
{
    if (!std::isfinite(dt) || dt <= 0 || !std::isfinite(level)) return;
    if (!mActive) {
        if (level < Threshold || !finite(earth.position) || !finite(earth.rotation)
            || !finite(earth.scale) || earth.scale.x <= 0 || earth.scale.y <= 0 || earth.scale.z <= 0) return;
        mActive = true; mEarth = earth;
        std::mt19937 rng(485);
        auto random = [&]() { return float(double(rng() >> 8)/16777216.0); };
        for (auto& f : mFragments) {
            f.transform = earth;
            f.transform.position = glm::vec3(earth.getMatrix()*glm::vec4(f.pivot,1));
            const auto outward = glm::normalize(f.transform.position-earth.position);
            f.velocity = outward * (6.0f+4.0f*random());
            f.angularVelocity = {random()-0.5f,random()-0.5f,random()-0.5f};
        }
        return; // First frame exactly reconstructs the crust; core is still hidden.
    }
    for (auto& f : mFragments) {
        f.transform.position += f.velocity*dt;
        f.transform.rotation += f.angularVelocity*dt;
    }
}
PointLight EarthBreakupSystem::coreLight() const
{
    PointLight light;
    light.position = mEarth.position; light.color = {1,0.8f,0.5f};
    light.intensity = mActive ? 450.0f : 0;
    light.constant = 1; light.linear = 0.05f; light.quadratic = 0.015f;
    return light;
}
void EarthBreakupSystem::publish(LightManager& lights) const
{
    if (!mActive) return;
    auto transient = lights.transientPointLights(); transient.push_back(coreLight());
    lights.setTransientPointLights(transient);
}
bool EarthBreakupSystem::initGraphics(const Mesh& earthMesh)
{
    if (mResources) return true;
    if (earthMesh.vertices().empty() || earthMesh.vertices().size()%3) return false;
    auto resources = std::make_unique<EarthBreakupResources>();
    if (!load(resources->coreShader,"shaders/breakup.vert","shaders/core.frag")
        || !load(resources->interiorShader,"shaders/breakup.vert","shaders/crust-interior.frag")) return false;
    std::vector<std::vector<Vertex>> exterior(FragmentCount), interior(FragmentCount);
    const auto& source = earthMesh.vertices();
    for (std::size_t i = 0; i < source.size(); i += 3) {
        const auto uv = (source[i].texCoords+source[i+1].texCoords+source[i+2].texCoords)/3.0f;
        const unsigned s = std::min(Sectors-1,unsigned(uv.x*Sectors));
        const unsigned b = std::min(Bands-1,unsigned((1-uv.y)*Bands));
        const unsigned index = b*Sectors+s;
        for (int j = 0; j < 3; ++j) {
            auto vertex = source[i+j]; vertex.position -= mFragments[index].pivot;
            exterior[index].push_back(vertex);
        }
        // Inner shell follows the same tessellation, with inward-facing normals.
        for (int j : {0,2,1}) {
            auto vertex = source[i+j]; vertex.position *= 0.72f;
            vertex.position -= mFragments[index].pivot; vertex.normal *= -1;
            interior[index].push_back(vertex);
        }
    }
    // Prepare radial walls along each patch boundary, matching the 32x32 sphere.
    for (unsigned b = 0; b < Bands; ++b) for (unsigned s = 0; s < Sectors; ++s) {
        const unsigned index = b*Sectors+s;
        auto wall = [&](glm::vec3 a, glm::vec3 c) {
            glm::vec3 points[] = {a,c,c*0.72f,a,c*0.72f,a*0.72f};
            const auto cross = glm::cross(c-a,c*0.72f-a);
            if (glm::length(cross) < 0.000001f) return;
            const auto normal = glm::normalize(cross);
            for (const auto& point : points) {
                Vertex v{}; v.position = point-mFragments[index].pivot; v.normal = normal;
                interior[index].push_back(v);
            }
        };
        for (unsigned j = 0; j < 8; ++j) {
            float v0 = float(b*8+j)/32, v1 = float(b*8+j+1)/32;
            wall(sphere(float(s)/Sectors,v0),sphere(float(s)/Sectors,v1));
            wall(sphere(float(s+1)/Sectors,v1),sphere(float(s+1)/Sectors,v0));
        }
        for (unsigned j = 0; j < 4; ++j) {
            float u0 = float(s*4+j)/32, u1 = float(s*4+j+1)/32;
            wall(sphere(u1,float(b)/Bands),sphere(u0,float(b)/Bands));
            wall(sphere(u0,float(b+1)/Bands),sphere(u1,float(b+1)/Bands));
        }
        auto outer = std::make_unique<Mesh>(), inner = std::make_unique<Mesh>();
        outer->setVertices(exterior[index]); inner->setVertices(interior[index]);
        resources->outer.push_back(std::move(outer)); resources->inner.push_back(std::move(inner));
    }
    resources->coreMaterial = Material(&resources->coreShader);
    resources->coreMaterial.setFloat("emission",20);
    resources->interiorMaterial = Material(&resources->interiorShader);
    resources->interiorMaterial.receivesLighting = true;
    mResources = std::move(resources);
    return true;
}
void EarthBreakupSystem::releaseGraphics() { mResources.reset(); }
void EarthBreakupSystem::render(Renderer& renderer, Material& crust, LightManager& lights,
    const glm::mat4& view, const glm::mat4& projection, const glm::vec3& eye)
{
    if (!mActive || !mResources) return;
    renderer.renderMesh(mResources->core.getMesh(),mResources->coreMaterial,mEarth,lights,view,projection,eye);
    for (unsigned i = 0; i < FragmentCount; ++i) {
        renderer.renderMesh(*mResources->outer[i],crust,mFragments[i].transform,lights,view,projection,eye);
        renderer.renderMesh(*mResources->inner[i],mResources->interiorMaterial,mFragments[i].transform,lights,view,projection,eye);
    }
}
