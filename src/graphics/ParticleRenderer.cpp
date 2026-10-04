#include "graphics/ParticleRenderer.h"
#include <cstddef>
#include <limits>
ParticleRenderer::~ParticleRenderer()
{
    if (mBuffer) glDeleteBuffers(1, &mBuffer);
    if (mVAO) glDeleteVertexArrays(1, &mVAO);
}
bool ParticleRenderer::init()
{
    if (mVAO) return true;
    if (!mShader.loadShaders("shaders/particle.vert", "shaders/particle.frag")) return false;
    GLint linked = GL_FALSE;
    glGetProgramiv(mShader.getProgram(), GL_LINK_STATUS, &linked);
    if (linked != GL_TRUE) return false;
    GLint vao, buffer;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &buffer);
    glGenVertexArrays(1, &mVAO);
    glGenBuffers(1, &mBuffer);
    glBindVertexArray(mVAO);
    glBindBuffer(GL_ARRAY_BUFFER, mBuffer);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Instance), nullptr);
    glVertexAttribDivisor(0, 1);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(Instance), reinterpret_cast<void*>(offsetof(Instance, opacity)));
    glVertexAttribDivisor(1, 1);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    return true;
}
void ParticleRenderer::render(const std::vector<Particle>& particles, const glm::mat4& view, const glm::mat4& projection)
{
    if (!mVAO || particles.empty() || particles.size() > std::size_t(std::numeric_limits<GLsizei>::max())) return;
    mInstances.clear();
    for (const auto& p : particles)
        mInstances.push_back({p.position.x, p.position.y, p.position.z, p.size, 1 - p.age / p.lifetime});
    GLint vao, buffer, program, srcRGB, dstRGB, srcAlpha, dstAlpha, eqRGB, eqAlpha, depthFunc;
    GLboolean depthWrite;
    const bool blend = glIsEnabled(GL_BLEND), depth = glIsEnabled(GL_DEPTH_TEST), cull = glIsEnabled(GL_CULL_FACE);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &buffer);
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);
    glGetIntegerv(GL_BLEND_SRC_RGB, &srcRGB); glGetIntegerv(GL_BLEND_DST_RGB, &dstRGB);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &srcAlpha); glGetIntegerv(GL_BLEND_DST_ALPHA, &dstAlpha);
    glGetIntegerv(GL_BLEND_EQUATION_RGB, &eqRGB); glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &eqAlpha);
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthWrite); glGetIntegerv(GL_DEPTH_FUNC, &depthFunc);
    glEnable(GL_DEPTH_TEST); glDepthFunc(GL_LESS); glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND); glBlendEquation(GL_FUNC_ADD); glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    mShader.use();
    mShader.setUniform("view", view); mShader.setUniform("projection", projection);
    glBindVertexArray(mVAO); glBindBuffer(GL_ARRAY_BUFFER, mBuffer);
    // Orphan the streaming buffer; one upload and one instanced draw for the population.
    glBufferData(GL_ARRAY_BUFFER, mInstances.size() * sizeof(Instance), mInstances.data(), GL_STREAM_DRAW);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, static_cast<GLsizei>(mInstances.size()));
    glBindVertexArray(vao); glBindBuffer(GL_ARRAY_BUFFER, buffer); glUseProgram(program);
    glBlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha); glBlendEquationSeparate(eqRGB, eqAlpha);
    if (!blend) glDisable(GL_BLEND);
    if (!depth) glDisable(GL_DEPTH_TEST);
    if (cull) glEnable(GL_CULL_FACE);
    glDepthMask(depthWrite); glDepthFunc(depthFunc);
}
