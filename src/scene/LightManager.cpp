#include "scene/LightManager.h"
#include <algorithm>
#include <cmath>

void LightManager::setTransientPointLights(const std::vector<PointLight>& lights)
{
    mTransientPointLights = lights;
}

std::vector<PointLight> LightManager::shaderPointLights() const
{
    auto selected = mPointLights;
    selected.insert(selected.end(), mTransientPointLights.begin(), mTransientPointLights.end());
    selected.erase(std::remove_if(selected.begin(), selected.end(), [](const PointLight& light)
                                  { return !std::isfinite(light.intensity) || light.intensity <= 0; }),
                   selected.end());
    std::stable_sort(selected.begin(), selected.end(),
                     [](const PointLight& a, const PointLight& b) { return a.intensity > b.intensity; });
    if (selected.size() > MaxPointLights)
    {
        selected.resize(MaxPointLights);
    }
    return selected;
}

void LightManager::setDirectionalLight(const DirectionalLight& light)
{
    mDirectionalLight = light;
}

const DirectionalLight& LightManager::getDirectionalLight() const
{
    return mDirectionalLight;
}

void LightManager::addPointLight(const PointLight& light)
{
    mPointLights.push_back(light);
}

const std::vector<PointLight>& LightManager::getPointLights() const
{
    return mPointLights;
}

void LightManager::applyToShader(ShaderProgram& shader) const
{

    // Soleil
    shader.setUniform("sunDirection", mDirectionalLight.direction);
    shader.setUniform("sunColor", mDirectionalLight.color);
    shader.setUniform("sunIntensity", mDirectionalLight.intensity);
    // Point lights
    const auto points = shaderPointLights();
    shader.setUniform("pointLightCount", static_cast<int>(points.size()));
    for (size_t i = 0; i < points.size(); ++i)
    {
        const PointLight& light = points[i];
        std::string index = std::to_string(i);
        shader.setUniform(("pointLights[" + index + "].position").c_str(), light.position);
        shader.setUniform(("pointLights[" + index + "].color").c_str(), light.color);
        shader.setUniform(("pointLights[" + index + "].intensity").c_str(), light.intensity);
        shader.setUniform(("pointLights[" + index + "].constant").c_str(), light.constant);
        shader.setUniform(("pointLights[" + index + "].linear").c_str(), light.linear);
        shader.setUniform(("pointLights[" + index + "].quadratic").c_str(), light.quadratic);
    }
}
