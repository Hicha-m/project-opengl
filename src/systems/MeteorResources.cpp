#include "systems/MeteorResources.h"

bool MeteorResources::load()
{
    if (!shader.loadShaders("shaders/meteor.vert", "shaders/meteor.frag"))
    {
        return false;
    }
    GLint linked = GL_FALSE;
    glGetProgramiv(shader.getProgram(), GL_LINK_STATUS, &linked);
    if (linked != GL_TRUE || !texture.loadTexture("textures/moon/2k_moon.jpg", true))
    {
        return false;
    }
    material.addTexture("rockMap", &texture, 0);
    material.receivesLighting = true;
    return true;
}
