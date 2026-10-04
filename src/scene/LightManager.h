#pragma once

#include <vector>

#include <glm/glm.hpp>

#include "graphics/ShaderProgram.h"


struct DirectionalLight
{
    glm::vec3 direction = glm::vec3(0.0f, -1.0f, 0.0f);

    glm::vec3 color = glm::vec3(1.0f);

    float intensity = 1.0f;
};


struct PointLight
{
    glm::vec3 position = glm::vec3(0.0f);

    glm::vec3 color = glm::vec3(1.0f);

    float intensity = 1.0f;

    float constant = 1.0f;

    float linear = 0.09f;

    float quadratic = 0.032f;
};


class LightManager
{
public:
    // Must match MAX_POINT_LIGHTS in earth.frag.
    static constexpr std::size_t MaxPointLights = 32;

    const std::vector<PointLight>& transientPointLights() const { return mTransientPointLights; }
    void setTransientPointLights(const std::vector<PointLight>& lights);
    // Highest intensity first; equal intensities retain insertion order.
    std::vector<PointLight> shaderPointLights() const;

    void setDirectionalLight(const DirectionalLight& light);

    const DirectionalLight& getDirectionalLight() const;

    void addPointLight(const PointLight& light);

    void clearPointLights();

    const std::vector<PointLight>& getPointLights() const;

    void applyToShader(ShaderProgram& shader) const;


private:

    DirectionalLight mDirectionalLight;

    std::vector<PointLight> mPointLights;
    std::vector<PointLight> mTransientPointLights;
};
