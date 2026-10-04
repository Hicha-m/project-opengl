#pragma once
#include <vector>
#include "graphics/ShaderProgram.h"
#include "systems/Particle.h"

class ParticleRenderer
{
public:
    ~ParticleRenderer();
    ParticleRenderer() = default;
    ParticleRenderer(const ParticleRenderer&) = delete;
    ParticleRenderer& operator=(const ParticleRenderer&) = delete;
    bool init();
    void render(const std::vector<Particle>& particles, const glm::mat4& view, const glm::mat4& projection);
private:
    struct Instance { float x, y, z, size, opacity; };
    ShaderProgram mShader;
    GLuint mVAO = 0, mBuffer = 0;
    std::vector<Instance> mInstances;
};
