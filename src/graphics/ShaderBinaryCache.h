#pragma once
#include <filesystem>
#include <glad/gl.h>
#include <string>

// Optional driver-specific optimization. All failures fall back to GLSL.
class ShaderBinaryCache
{
public:
    explicit ShaderBinaryCache(const std::string& vertex, const std::string& fragment);
    bool supported() const { return mGet && mLoad && mParameter; }
    bool load(GLuint program) const;
    void prepare(GLuint program) const;
    void save(GLuint program) const;

private:
    using Get = void(GLAD_API_PTR*)(GLuint, GLsizei, GLsizei*, GLenum*, void*);
    using Load = void(GLAD_API_PTR*)(GLuint, GLenum, const void*, GLsizei);
    using Parameter = void(GLAD_API_PTR*)(GLuint, GLenum, GLint);
    Get mGet = nullptr;
    Load mLoad = nullptr;
    Parameter mParameter = nullptr;
    std::string mIdentity;
    std::filesystem::path mPath;
};
