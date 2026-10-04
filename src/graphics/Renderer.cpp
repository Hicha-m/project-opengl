#include "graphics/Renderer.h"

#include <GL/glew.h>


void Renderer::render(
    Scene& scene,
    LightManager& lightManager,
    const glm::mat4& view,
    const glm::mat4& projection,
    const glm::vec3& cameraPosition
)
{
    for (SceneObject& object : scene.objects)
    {
        renderObject(
            object,
            lightManager,
            view,
            projection,
            cameraPosition
        );
    }
}


void Renderer::renderObject(
    SceneObject& object,
    LightManager& lightManager,
    const glm::mat4& view,
    const glm::mat4& projection,
    const glm::vec3& cameraPosition
)
{
    if (!object.visible || !object.isValid())
        return;


    renderMesh(*object.mesh, object.material, object.transform,
        lightManager, view, projection, cameraPosition);
}

void Renderer::renderMesh(
    Mesh& mesh, Material& material, const Transform& transform,
    LightManager& lightManager, const glm::mat4& view,
    const glm::mat4& projection, const glm::vec3& cameraPosition)
{
    if (!material.shader) return;

    ShaderProgram* shader = material.shader;


    // --------------------------------------------------
    // Depth
    // --------------------------------------------------

    glDepthFunc(
        material.depthLEqual
            ? GL_LEQUAL
            : GL_LESS
    );


    glDepthMask(
        material.depthWrite
            ? GL_TRUE
            : GL_FALSE
    );


    // --------------------------------------------------
    // Blending
    // --------------------------------------------------

    if (material.blending)
    {
        glEnable(GL_BLEND);

        glBlendFunc(
            GL_SRC_ALPHA,
            GL_ONE_MINUS_SRC_ALPHA
        );
    }
    else
    {
        glDisable(GL_BLEND);
    }


    // --------------------------------------------------
    // Shader
    // --------------------------------------------------

    shader->use();


    shader->setUniform(
        "model",
        transform.getMatrix()
    );


    shader->setUniform(
        "view",
        view
    );


    shader->setUniform(
        "projection",
        projection
    );


    shader->setUniform(
        "viewPos",
        cameraPosition
    );


    // --------------------------------------------------
    // Lighting
    // --------------------------------------------------

    if (material.receivesLighting)
    {
        lightManager.applyToShader(
            *shader
        );
    }


    // --------------------------------------------------
    // Custom float uniforms
    // --------------------------------------------------
    for (auto& pair : material.floatUniforms)
    {
        shader->setUniform(
            pair.first.c_str(),
            pair.second
        );
    }


    // --------------------------------------------------
    // Custom vec3 uniforms
    // --------------------------------------------------

    for (auto& pair : material.vec3Uniforms)
    {
        shader->setUniform(
            pair.first.c_str(),
            pair.second
        );
    }


    // --------------------------------------------------
    // Custom int uniforms
    // --------------------------------------------------

    for (auto& pair : material.intUniforms)
    {
        shader->setUniform(
            pair.first.c_str(),
            pair.second
        );
    }

    // --------------------------------------------------
    // Textures
    // --------------------------------------------------

    for (auto& pair : material.textures)
    {
        MaterialTexture& texture =
            pair.second;


        if (texture.texture == nullptr)
            continue;


        texture.texture->bind(
            texture.unit
        );


        shader->setUniformSampler(
            pair.first.c_str(),
            texture.unit
        );
    }


    // --------------------------------------------------
    // Draw
    // --------------------------------------------------

    mesh.draw();


    // --------------------------------------------------
    // Unbind textures
    // --------------------------------------------------

    for (auto& pair : material.textures)
    {
        MaterialTexture& texture =
            pair.second;


        if (texture.texture == nullptr)
            continue;


        texture.texture->unbind(
            texture.unit
        );
    }


    // --------------------------------------------------
    // Restore OpenGL state
    // --------------------------------------------------

    glDisable(GL_BLEND);

    glDepthMask(GL_TRUE);

    glDepthFunc(GL_LESS);

    glEnable(GL_DEPTH_TEST);
}
