#include "graphics/ShaderBinaryCache.h"
#include "platform/StartupProfile.h"
#include <GLFW/glfw3.h>
#include <SDL3/SDL.h>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>

namespace
{
    constexpr GLenum retrievableHint = 0x8257, binaryLength = 0x8741, binaryFormats = 0x87FE;
    constexpr std::size_t maxBinary = 16 * 1024 * 1024;
    std::uint64_t hash(const std::string& text)
    {
        std::uint64_t value = 14695981039346656037ull;
        for (unsigned char c : text)
        {
            value ^= c;
            value *= 1099511628211ull;
        }
        return value;
    }
    void write32(std::ostream& out, std::uint32_t n)
    {
        for (unsigned i = 0; i < 4; ++i)
        {
            out.put(static_cast<char>((n >> (i * 8)) & 255));
        }
    }
    std::uint32_t read32(std::istream& in)
    {
        std::uint32_t n = 0;
        for (unsigned i = 0; i < 4; ++i)
        {
            const int c = in.get();
            if (c == EOF)
            {
                throw std::runtime_error("Truncated shader cache");
            }
            n |= std::uint32_t(c) << (i * 8);
        }
        return n;
    }
    bool binarySupport()
    {
        GLint major = 0, minor = 0;
        glGetIntegerv(GL_MAJOR_VERSION, &major);
        glGetIntegerv(GL_MINOR_VERSION, &minor);
        if (major > 4 || (major == 4 && minor >= 1))
        {
            return true;
        }
        GLint count = 0;
        glGetIntegerv(GL_NUM_EXTENSIONS, &count);
        for (GLint i = 0; i < count; ++i)
        {
            const auto* extension = glGetStringi(GL_EXTENSIONS, i);
            if (extension &&
                std::string(reinterpret_cast<const char*>(extension)) == "GL_ARB_get_program_binary")
            {
                return true;
            }
        }
        return false;
    }
} // namespace

ShaderBinaryCache::ShaderBinaryCache(const std::string& vertex, const std::string& fragment)
{
    if (StartupProfile::disableShaderCache || !binarySupport())
    {
        return;
    }
    GLint formats = 0;
    glGetIntegerv(binaryFormats, &formats);
    if (formats <= 0)
    {
        return;
    }
    mGet = reinterpret_cast<Get>(glfwGetProcAddress("glGetProgramBinary"));
    mLoad = reinterpret_cast<Load>(glfwGetProcAddress("glProgramBinary"));
    mParameter = reinterpret_cast<Parameter>(glfwGetProcAddress("glProgramParameteri"));
    if (!supported())
    {
        return;
    }
    try
    {
        mIdentity = "SpaceShaderCache-v1";
        for (GLenum field : {GL_VENDOR, GL_RENDERER, GL_VERSION, GL_SHADING_LANGUAGE_VERSION})
        {
            const auto* s = glGetString(field);
            mIdentity.push_back('\0');
            if (s)
            {
                mIdentity += reinterpret_cast<const char*>(s);
            }
        }
        mIdentity.push_back('\0');
        mIdentity += vertex;
        mIdentity.push_back('\0');
        mIdentity += fragment;
        char* directory = SDL_GetPrefPath("OpenGLSpaceCinematic", "ShaderCache");
        if (!directory)
        {
            mGet = nullptr;
            return;
        }
        const auto root = std::filesystem::u8path(directory);
        SDL_free(directory);
        std::ostringstream name;
        name << std::hex << hash(mIdentity) << ".bin";
        mPath = root / name.str();
    }
    catch (...)
    {
        mGet = nullptr;
    }
}

void ShaderBinaryCache::prepare(GLuint program) const
{
    if (supported())
    {
        mParameter(program, retrievableHint, GL_TRUE);
    }
}
bool ShaderBinaryCache::load(GLuint program) const
{
    if (!supported())
    {
        return false;
    }
    try
    {
        std::ifstream in(mPath, std::ios::binary | std::ios::ate);
        if (!in || in.tellg() > static_cast<std::streamoff>(maxBinary + mIdentity.size() + 32))
        {
            return false;
        }
        in.seekg(0);
        if (read32(in) != 0x43424c47 || read32(in) != 1 || read32(in) != mIdentity.size())
        {
            return false;
        }
        std::string identity(mIdentity.size(), '\0');
        if (!in.read(identity.data(), identity.size()) || identity != mIdentity)
        {
            return false;
        }
        const auto format = read32(in), size = read32(in), low = read32(in), high = read32(in);
        if (!size || size > maxBinary)
        {
            return false;
        }
        std::string binary(size, '\0');
        if (!in.read(binary.data(), size) || in.peek() != EOF ||
            hash(binary) != (std::uint64_t(high) << 32 | low))
        {
            return false;
        }
        GLint count = 0;
        glGetIntegerv(binaryFormats, &count);
        if (count <= 0)
        {
            return false;
        }
        std::vector<GLint> formats(count);
        glGetIntegerv(0x87FF, formats.data());
        if (std::find(formats.begin(), formats.end(), static_cast<GLint>(format)) == formats.end())
        {
            return false;
        }
        mLoad(program, format, binary.data(), static_cast<GLsizei>(size));
        GLint linked = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linked);
        return linked == GL_TRUE;
    }
    catch (...)
    {
        return false;
    }
}
void ShaderBinaryCache::save(GLuint program) const
{
    if (!supported())
    {
        return;
    }
    std::filesystem::path temporary;
    try
    {
        GLint length = 0;
        glGetProgramiv(program, binaryLength, &length);
        if (length <= 0 || std::size_t(length) > maxBinary)
        {
            return;
        }
        std::string binary(length, '\0');
        GLenum format = 0;
        GLsizei size = 0;
        mGet(program, length, &size, &format, binary.data());
        if (size <= 0 || size > length)
        {
            return;
        }
        binary.resize(size);
        temporary = mPath;
        temporary += "." + std::to_string(SDL_GetTicksNS()) + ".tmp";
        std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
        write32(out, 0x43424c47);
        write32(out, 1);
        write32(out, static_cast<std::uint32_t>(mIdentity.size()));
        out.write(mIdentity.data(), mIdentity.size());
        write32(out, format);
        write32(out, size);
        const auto checksum = hash(binary);
        write32(out, static_cast<std::uint32_t>(checksum));
        write32(out, static_cast<std::uint32_t>(checksum >> 32));
        out.write(binary.data(), size);
        out.close();
        if (!out)
        {
            throw std::runtime_error("Cannot write shader cache");
        }
#ifdef _WIN32
        std::error_code ec;
        std::filesystem::remove(mPath, ec);
#endif
        std::filesystem::rename(temporary, mPath);
    }
    catch (...)
    {
        std::error_code ignored;
        if (!temporary.empty())
        {
            std::filesystem::remove(temporary, ignored);
        }
    }
}
