#include "LightManager.h"

void LightManager::setDirectionalLight(const DirectionalLight &light)
{
    mDirectionalLight = light;
}

const DirectionalLight &LightManager::getDirectionalLight() const
{
    return mDirectionalLight;
}

void LightManager::addPointLight(const PointLight &light)
{
    mPointLights.push_back(light);
}

void LightManager::clearPointLights()
{
    mPointLights.clear();
}

const std::vector<PointLight> &LightManager::getPointLights() const
{
    return mPointLights;
}

void LightManager::applyToShader(
    ShaderProgram &shader) const
{
    // --------------------------------------------------
    // Soleil
    // --------------------------------------------------

    shader.setUniform(
        "sunDirection",
        mDirectionalLight.direction);

    shader.setUniform(
        "sunColor",
        mDirectionalLight.color);

    shader.setUniform(
        "sunIntensity",
        mDirectionalLight.intensity);

    // --------------------------------------------------
    // Point lights
    // --------------------------------------------------

    shader.setUniform(
        "pointLightCount",
        static_cast<int>(mPointLights.size()));

    for (size_t i = 0;
         i < mPointLights.size();
         ++i)
    {
        const PointLight &light =
            mPointLights[i];

        std::string index =
            std::to_string(i);

        shader.setUniform(
            ("pointLights[" + index + "].position").c_str(),
            light.position);

        shader.setUniform(
            ("pointLights[" + index + "].color").c_str(),
            light.color);

        shader.setUniform(
            ("pointLights[" + index + "].intensity").c_str(),
            light.intensity);

        shader.setUniform(
            ("pointLights[" + index + "].constant").c_str(),
            light.constant);

        shader.setUniform(
            ("pointLights[" + index + "].linear").c_str(),
            light.linear);

        shader.setUniform(
            ("pointLights[" + index + "].quadratic").c_str(),
            light.quadratic);
    }
}