//-----------------------------------------------------------------------------
// Simple 2D texture class
//-----------------------------------------------------------------------------
#include "graphics/Texture2D.h"
#include "assets/PreparedAssets.h"
#include "platform/ResourcePaths.h"
#include "platform/StartupProfile.h"
#include <GLFW/glfw3.h>
#include <cassert>
#include <iostream>
#include <map>
#include <tuple>

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
Texture2D::Texture2D() : mTexture(0)
{
}

//-----------------------------------------------------------------------------
// Destructor
//-----------------------------------------------------------------------------
Texture2D::~Texture2D()
{
    if (!mSharedTexture)
    {
        glDeleteTextures(1, &mTexture);
    }
}

//-----------------------------------------------------------------------------
// Load build-prepared RGBA pixels; immutable GPU images are shared within a context.
// Creates mip maps if generateMipMaps is true.
//-----------------------------------------------------------------------------
bool Texture2D::loadTexture(const string& fileName, bool generateMipMaps)
{
    StartupProfile::Timer timer(StartupProfile::Textures);
    // Weak references never keep GPU objects alive beyond their owning context.
    using Key = std::tuple<GLFWwindow*, std::filesystem::path, bool>;
    static std::map<Key, std::weak_ptr<GLuint>> images;
    for (auto it = images.begin(); it != images.end();)
    {
        if (it->second.expired())
        {
            it = images.erase(it);
        }
        else
        {
            ++it;
        }
    }
    const auto path = ResourcePaths::resolve("build/assets/" + fileName + ".rgba");
    const Key key{glfwGetCurrentContext(), path, generateMipMaps};
    auto install = [&](std::shared_ptr<GLuint> image)
    {
        if (!mSharedTexture && mTexture)
        {
            glDeleteTextures(1, &mTexture);
        }
        mSharedTexture = std::move(image);
        mTexture = *mSharedTexture;
        mRedWidth = mRedHeight = 0;
    };
    if (auto shared = images[key].lock())
    {
        install(std::move(shared));
        ++StartupProfile::textureHits;
        return true;
    }
    try
    {
        const auto data = readTexture(path);
        GLint limit = 0;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &limit);
        if (data.width > unsigned(limit) || data.height > unsigned(limit))
        {
            throw std::runtime_error("Texture exceeds GPU size limit");
        }
        GLint binding = 0, alignment = 0;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &binding);
        glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment);
        GLuint id = 0;
        glGenTextures(1, &id);
        auto shared = std::shared_ptr<GLuint>(new GLuint(id),
                                              [](GLuint* texture)
                                              {
                                                  glDeleteTextures(1, texture);
                                                  delete texture;
                                              });
        glBindTexture(GL_TEXTURE_2D, id);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, data.width, data.height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                     data.pixels.data());
        if (generateMipMaps)
        {
            glGenerateMipmap(GL_TEXTURE_2D);
        }
        glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
        glBindTexture(GL_TEXTURE_2D, binding);
        images[key] = shared;
        install(std::move(shared));
        return true;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Cannot load prepared texture " << fileName << ": " << error.what() << '\n';
        return false;
    }
}

//-----------------------------------------------------------------------------
// Bind the texture unit passed in as the active texture in the shader
//-----------------------------------------------------------------------------
void Texture2D::bind(GLuint texUnit)
{
    assert(texUnit < 32);

    glActiveTexture(GL_TEXTURE0 + texUnit);
    glBindTexture(GL_TEXTURE_2D, mTexture);
}

//-----------------------------------------------------------------------------
// Unbind the texture unit passed in as the active texture in the shader
//-----------------------------------------------------------------------------
void Texture2D::unbind(GLuint texUnit)
{
    glActiveTexture(GL_TEXTURE0 + texUnit);
    glBindTexture(GL_TEXTURE_2D, 0);
}

// Preserve the caller's active unit, binding and pixel packing settings.
bool Texture2D::createRed(int width, int height, const float* pixels, bool floatingPoint)
{
    if (width <= 0 || height <= 0 || !pixels)
    {
        return false;
    }
    if (mSharedTexture)
    {
        mSharedTexture.reset();
        mTexture = 0;
    }
    GLint binding, alignment;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &binding);
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment);
    if (!mTexture)
    {
        glGenTextures(1, &mTexture);
    }
    glBindTexture(GL_TEXTURE_2D, mTexture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, floatingPoint ? GL_R32F : GL_R8, width, height, 0, GL_RED, GL_FLOAT,
                 pixels);
    glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
    glBindTexture(GL_TEXTURE_2D, binding);
    mRedWidth = width;
    mRedHeight = height;
    return true;
}
bool Texture2D::updateRed(int width, int height, const float* pixels)
{
    if (!mTexture || width != mRedWidth || height != mRedHeight || !pixels)
    {
        return false;
    }
    GLint binding, alignment;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &binding);
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment);
    glBindTexture(GL_TEXTURE_2D, mTexture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RED, GL_FLOAT, pixels);
    glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
    glBindTexture(GL_TEXTURE_2D, binding);
    return true;
}
