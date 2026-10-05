//-----------------------------------------------------------------------------
// GLSL shader manager class
//-----------------------------------------------------------------------------
#include "graphics/ShaderProgram.h"
#include "assets/PreparedAssets.h"
#include "graphics/ShaderBinaryCache.h"
#include "platform/ResourcePaths.h"
#include "platform/StartupProfile.h"
#include <iostream>

#include <glm/gtc/type_ptr.hpp>

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
ShaderProgram::ShaderProgram() : mHandle(0)
{
}

//-----------------------------------------------------------------------------
// Destructor
//-----------------------------------------------------------------------------
ShaderProgram::~ShaderProgram()
{
    // Delete the program
    if (mHandle)
    {
        GLint current = 0;
        glGetIntegerv(GL_CURRENT_PROGRAM, &current);
        if (GLuint(current) == mHandle)
        {
            glUseProgram(0);
        }
        glDeleteProgram(mHandle);
    }
}

//-----------------------------------------------------------------------------
// Loads vertex and fragment shaders
//-----------------------------------------------------------------------------
bool ShaderProgram::loadShaders(const char* vsFilename, const char* fsFilename)
{
    StartupProfile::Timer timer(StartupProfile::Shaders);
    mLoadedFromCache = false;
    try
    {
        // Read the archive once per resource root; never retain failed reads.
        static std::filesystem::path root;
        static ShaderSources sources;
        const auto path = ResourcePaths::resolve("build/assets/shaders.bin");
        const auto currentStamp = std::filesystem::last_write_time(path);
        static std::filesystem::file_time_type stamp;
        if (path != root || currentStamp != stamp)
        {
            auto prepared = readShaders(path);
            sources = std::move(prepared);
            root = path;
            stamp = currentStamp;
        }
        for (const auto* name : {vsFilename, fsFilename})
        {
            if (!sources.count(name))
            {
                throw std::runtime_error(std::string("Prepared archive lacks ") + name + " (rebuild assets)");
            }
        }
        const auto& vertex = sources.at(vsFilename);
        const auto& fragment = sources.at(fsFilename);
        if (vertex.empty() || fragment.empty())
        {
            throw std::runtime_error("Empty shader source");
        }
        ShaderBinaryCache cache(vertex, fragment);
        const GLuint program = glCreateProgram();
        if (!program)
        {
            return false;
        }
        auto commit = [&]()
        {
            if (mHandle)
            {
                glDeleteProgram(mHandle);
            }
            mHandle = program;
            mUniformLocations.clear();
        };
        if (cache.load(program))
        {
            commit();
            mLoadedFromCache = true;
            ++StartupProfile::shaderHits;
            return true;
        }
        ++StartupProfile::shaderCompiles;
        cache.prepare(program);
        const GLuint vs = glCreateShader(GL_VERTEX_SHADER), fs = glCreateShader(GL_FRAGMENT_SHADER);
        if (!vs || !fs)
        {
            glDeleteShader(vs);
            glDeleteShader(fs);
            glDeleteProgram(program);
            return false;
        }
        const GLchar* v = vertex.c_str();
        const GLchar* f = fragment.c_str();
        glShaderSource(vs, 1, &v, nullptr);
        glShaderSource(fs, 1, &f, nullptr);
        glCompileShader(vs);
        glCompileShader(fs);
        GLint vOK = GL_FALSE, fOK = GL_FALSE;
        glGetShaderiv(vs, GL_COMPILE_STATUS, &vOK);
        glGetShaderiv(fs, GL_COMPILE_STATUS, &fOK);
        if (!vOK || !fOK)
        {
            std::cerr << "Shader compilation failed: " << vsFilename << ", " << fsFilename << '\n';
            checkCompileErrors(vs, VERTEX);
            checkCompileErrors(fs, FRAGMENT);
            glDeleteShader(vs);
            glDeleteShader(fs);
            glDeleteProgram(program);
            return false;
        }
        glAttachShader(program, vs);
        glAttachShader(program, fs);
        glLinkProgram(program);
        glDeleteShader(vs);
        glDeleteShader(fs);
        GLint linked = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linked);
        if (!linked)
        {
            checkCompileErrors(program, PROGRAM);
            glDeleteProgram(program);
            return false;
        }
        cache.save(program);
        commit();
        return true;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Cannot load prepared shaders " << vsFilename << ", " << fsFilename << ": "
                  << error.what() << '\n';
        return false;
    }
}

//-----------------------------------------------------------------------------
// Activate the shader program
//-----------------------------------------------------------------------------
void ShaderProgram::use()
{
    if (mHandle > 0)
    {
        glUseProgram(mHandle);
    }
}

//-----------------------------------------------------------------------------
// Checks for shader compiler errors
//-----------------------------------------------------------------------------
void ShaderProgram::checkCompileErrors(GLuint shader, ShaderType type)
{
    int status = 0;

    if (type == PROGRAM)
    {
        glGetProgramiv(shader, GL_LINK_STATUS, &status);
        if (status == GL_FALSE)
        {
            GLint length = 0;
            glGetProgramiv(shader, GL_INFO_LOG_LENGTH, &length);

            // The length includes the NULL character
            string errorLog(length, ' '); // Resize and fill with space character
            glGetProgramInfoLog(shader, length, &length, &errorLog[0]);
            std::cerr << "Error! Shader program failed to link. " << errorLog << std::endl;
        }
    }
    else
    {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
        if (status == GL_FALSE)
        {
            GLint length = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

            // The length includes the NULL character
            string errorLog(length, ' '); // Resize and fill with space character
            glGetShaderInfoLog(shader, length, &length, &errorLog[0]);
            std::cerr << "Error! Shader failed to compile. " << errorLog << std::endl;
        }
    }
}

//-----------------------------------------------------------------------------
// Returns the active shader program
//-----------------------------------------------------------------------------
GLuint ShaderProgram::getProgram() const
{
    return mHandle;
}

//-----------------------------------------------------------------------------
// Sets a glm::vec2 shader uniform
//-----------------------------------------------------------------------------
void ShaderProgram::setUniform(const GLchar* name, const glm::vec2& v)
{
    GLint loc = getUniformLocation(name);
    glUniform2f(loc, v.x, v.y);
}

//-----------------------------------------------------------------------------
// Sets a glm::vec3 shader uniform
//-----------------------------------------------------------------------------
void ShaderProgram::setUniform(const GLchar* name, const glm::vec3& v)
{
    GLint loc = getUniformLocation(name);
    glUniform3f(loc, v.x, v.y, v.z);
}

//-----------------------------------------------------------------------------
// Sets a glm::vec4 shader uniform
//-----------------------------------------------------------------------------
void ShaderProgram::setUniform(const GLchar* name, const glm::vec4& v)
{
    GLint loc = getUniformLocation(name);
    glUniform4f(loc, v.x, v.y, v.z, v.w);
}

//-----------------------------------------------------------------------------
// Sets a glm::mat4 shader uniform
//-----------------------------------------------------------------------------
void ShaderProgram::setUniform(const GLchar* name, const glm::mat4& m)
{
    GLint loc = getUniformLocation(name);

    // loc = location of uniform in shader
    // count = how many matrices (1 if not an array of mats)
    // transpose = False for opengl because column major
    // value = the matrix to set for the uniform
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(m));
}

//-----------------------------------------------------------------------------
// Sets a GLfloat shader uniform
//-----------------------------------------------------------------------------
void ShaderProgram::setUniform(const GLchar* name, const GLfloat f)
{
    GLint loc = getUniformLocation(name);
    glUniform1f(loc, f);
}

//-----------------------------------------------------------------------------
// Sets a GLint shader uniform
//-----------------------------------------------------------------------------
void ShaderProgram::setUniform(const GLchar* name, const GLint v)
{
    GLint loc = getUniformLocation(name);
    glUniform1i(loc, v);
}

//-----------------------------------------------------------------------------
// Sets a GLint shader uniform that is specific to a texture unit
//-----------------------------------------------------------------------------
void ShaderProgram::setUniformSampler(const GLchar* name, const GLint& slot)
{
    glActiveTexture(GL_TEXTURE0 + slot);

    GLint loc = getUniformLocation(name);
    glUniform1i(loc, slot);
}

//-----------------------------------------------------------------------------
// Returns the uniform identifier given it's string name.
// NOTE: Shader must be currently active first.
//-----------------------------------------------------------------------------
GLint ShaderProgram::getUniformLocation(const GLchar* name)
{
    std::map<string, GLint>::iterator it = mUniformLocations.find(name);

    // Only need to query the shader program IF it doesn't already exist.
    if (it == mUniformLocations.end())
    {
        // Find it and add it to the map
        mUniformLocations[name] = glGetUniformLocation(mHandle, name);
    }

    // Return it
    return mUniformLocations[name];
}
