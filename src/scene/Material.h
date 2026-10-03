#pragma once

#include <unordered_map>
#include <string>

#include <glm/glm.hpp>

#include "ShaderProgram.h"
#include "Texture2D.h"


struct MaterialTexture
{
    Texture2D* texture = nullptr;
    unsigned int unit = 0;
};


class Material
{
public:

    ShaderProgram* shader = nullptr;


    // --------------------------------------------------
    // Textures
    // --------------------------------------------------

    std::unordered_map<std::string, MaterialTexture> textures;


    // --------------------------------------------------
    // Uniforms
    // --------------------------------------------------

    std::unordered_map<std::string, float> floatUniforms;

    std::unordered_map<std::string, glm::vec3> vec3Uniforms;

    std::unordered_map<std::string, int> intUniforms;


    // --------------------------------------------------
    // Render state
    // --------------------------------------------------

    bool blending = false;

    bool depthLEqual = false;

    bool depthWrite = true;


    // --------------------------------------------------
    // Lighting
    // --------------------------------------------------

    bool receivesLighting = false;


    Material() = default;


    Material(ShaderProgram* shader)
        : shader(shader)
    {
    }


    void addTexture(
        const std::string& uniformName,
        Texture2D* texture,
        unsigned int unit
    )
    {
        textures[uniformName] = {
            texture,
            unit
        };
    }


    void setFloat(
        const std::string& uniformName,
        float value
    )
    {
        floatUniforms[uniformName] = value;
    }


    void setVec3(
        const std::string& uniformName,
        const glm::vec3& value
    )
    {
        vec3Uniforms[uniformName] = value;
    }


    void setInt(
        const std::string& uniformName,
        int value
    )
    {
        intUniforms[uniformName] = value;
    }
};