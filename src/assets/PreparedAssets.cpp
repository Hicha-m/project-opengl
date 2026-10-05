#include "assets/PreparedAssets.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace
{
    // Fixed little-endian integers and IEEE754 floats; never serialize GLM structs.
    constexpr std::size_t maxFile = 512 * 1024 * 1024;
    struct Writer
    {
        std::vector<unsigned char> bytes;
        void u32(std::uint32_t v)
        {
            for (unsigned i = 0; i < 4; ++i)
            {
                bytes.push_back((v >> (i * 8)) & 255);
            }
        }
        void number(float v)
        {
            static_assert(sizeof(float) == 4 && std::numeric_limits<float>::is_iec559);
            if (!std::isfinite(v))
            {
                throw std::runtime_error("Non-finite asset value");
            }
            std::uint32_t bits;
            std::memcpy(&bits, &v, 4);
            u32(bits);
        }
        void string(const std::string& v)
        {
            u32(static_cast<std::uint32_t>(v.size()));
            bytes.insert(bytes.end(), v.begin(), v.end());
        }
        void save(const std::filesystem::path& path, std::uint32_t magic)
        {
            std::filesystem::create_directories(path.parent_path());
            Writer header;
            header.u32(magic);
            header.u32(1);
            std::ofstream out(path, std::ios::binary | std::ios::trunc);
            out.write(reinterpret_cast<const char*>(header.bytes.data()), header.bytes.size());
            out.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
            out.close();
            if (!out)
            {
                throw std::runtime_error("Cannot write prepared asset: " + path.string());
            }
        }
    };
    struct Reader
    {
        std::vector<unsigned char> bytes;
        std::size_t cursor = 0;
        explicit Reader(const std::filesystem::path& path, std::uint32_t magic)
        {
            std::ifstream in(path, std::ios::binary | std::ios::ate);
            const auto size = in.tellg();
            if (!in || size < 8 || size > static_cast<std::streamoff>(maxFile))
            {
                throw std::runtime_error("Missing or invalid prepared asset: " + path.string() +
                                         " (rebuild assets)");
            }
            bytes.resize(static_cast<std::size_t>(size));
            in.seekg(0);
            if (!in.read(reinterpret_cast<char*>(bytes.data()), bytes.size()))
            {
                throw std::runtime_error("Cannot read asset");
            }
            if (u32() != magic || u32() != 1)
            {
                throw std::runtime_error("Invalid prepared asset format: " + path.string());
            }
        }
        void need(std::size_t n) const
        {
            if (n > bytes.size() - cursor)
            {
                throw std::runtime_error("Truncated prepared asset");
            }
        }
        std::uint32_t u32()
        {
            need(4);
            std::uint32_t v = 0;
            for (unsigned i = 0; i < 4; ++i)
            {
                v |= std::uint32_t(bytes[cursor++]) << (8 * i);
            }
            return v;
        }
        float number()
        {
            auto bits = u32();
            float v;
            std::memcpy(&v, &bits, 4);
            if (!std::isfinite(v))
            {
                throw std::runtime_error("Invalid asset float");
            }
            return v;
        }
        std::string string()
        {
            const auto n = u32();
            need(n);
            std::string v(bytes.begin() + cursor, bytes.begin() + cursor + n);
            cursor += n;
            return v;
        }
        void done() const
        {
            if (cursor != bytes.size())
            {
                throw std::runtime_error("Unexpected prepared asset data");
            }
        }
    };
    constexpr std::uint32_t shuttleMagic = 0x4c544853, textureMagic = 0x41424752, shaderMagic = 0x4c534c47;
} // namespace

ShuttleData prepareShuttle(const std::filesystem::path& obj, const std::filesystem::path& mtl)
{
    ShuttleData data;
    data.geometry = readOBJ(obj);
    std::ifstream file(mtl);
    if (!file)
    {
        throw std::runtime_error("Cannot open MTL: " + mtl.string());
    }
    std::string line, name;
    while (std::getline(file, line))
    {
        std::istringstream row(line);
        std::string key;
        row >> key;
        if (key == "newmtl")
        {
            row >> name;
            data.surfaces[name];
        }
        else if (!name.empty())
        {
            auto& material = data.surfaces[name];
            if (key == "Kd")
            {
                row >> material.color.x >> material.color.y >> material.color.z;
            }
            else if (key == "Pm")
            {
                row >> material.metallic;
            }
            else if (key == "Pr")
            {
                row >> material.roughness;
            }
            else if (key == "map_Kd")
            {
                row >> material.diffuse;
            }
            else if (key == "norm")
            {
                row >> material.normal;
            }
        }
    }
    auto& vertices = data.geometry.vertices;
    glm::vec3 low(vertices.front().position), high(low);
    for (const auto& v : vertices)
    {
        low = glm::min(low, v.position);
        high = glm::max(high, v.position);
    }
    const auto center = (low + high) * 0.5f, extent = high - low;
    const float size = std::max({extent.x, extent.y, extent.z});
    if (!(size > 0))
    {
        throw std::runtime_error("Degenerate shuttle geometry");
    }
    for (auto& v : vertices)
    {
        v.position = (v.position - center) / size;
    }
    for (const auto& section : data.geometry.sections)
    {
        data.surfaces[section.material];
    }
    return data;
}

void writeShuttle(const std::filesystem::path& path, const ShuttleData& data)
{
    Writer out;
    out.u32(static_cast<std::uint32_t>(data.geometry.vertices.size()));
    for (const auto& v : data.geometry.vertices)
    {
        for (unsigned i = 0; i < 3; ++i)
        {
            out.number(v.position[i]);
        }
        for (unsigned i = 0; i < 3; ++i)
        {
            out.number(v.normal[i]);
        }
        for (unsigned i = 0; i < 2; ++i)
        {
            out.number(v.texCoords[i]);
        }
        for (unsigned i = 0; i < 3; ++i)
        {
            out.number(v.tangent[i]);
        }
    }
    out.u32(static_cast<std::uint32_t>(data.geometry.sections.size()));
    for (const auto& s : data.geometry.sections)
    {
        out.string(s.material);
        out.u32(static_cast<std::uint32_t>(s.first));
        out.u32(static_cast<std::uint32_t>(s.count));
        const auto& m = data.surfaces.at(s.material);
        for (unsigned i = 0; i < 3; ++i)
        {
            out.number(m.color[i]);
        }
        out.number(m.metallic);
        out.number(m.roughness);
        out.string(m.diffuse);
        out.string(m.normal);
    }
    out.save(path, shuttleMagic);
}

ShuttleData readShuttle(const std::filesystem::path& path)
{
    Reader in(path, shuttleMagic);
    ShuttleData data;
    const auto n = in.u32();
    if (!n || n % 3)
    {
        throw std::runtime_error("Invalid shuttle vertex count");
    }
    in.need(std::size_t(n) * 44);
    data.geometry.vertices.resize(n);
    for (auto& v : data.geometry.vertices)
    {
        for (unsigned i = 0; i < 3; ++i)
        {
            v.position[i] = in.number();
        }
        for (unsigned i = 0; i < 3; ++i)
        {
            v.normal[i] = in.number();
        }
        for (unsigned i = 0; i < 2; ++i)
        {
            v.texCoords[i] = in.number();
        }
        for (unsigned i = 0; i < 3; ++i)
        {
            v.tangent[i] = in.number();
        }
    }
    const auto sections = in.u32();
    if (!sections || sections > n / 3)
    {
        throw std::runtime_error("Invalid shuttle section count");
    }
    std::size_t next = 0;
    for (unsigned i = 0; i < sections; ++i)
    {
        MeshSection s;
        s.material = in.string();
        s.first = in.u32();
        s.count = in.u32();
        if (s.first != next || !s.count || s.count % 3 || s.count > n - next)
        {
            throw std::runtime_error("Invalid shuttle section");
        }
        next += s.count;
        ShuttleSurface m;
        for (unsigned j = 0; j < 3; ++j)
        {
            m.color[j] = in.number();
        }
        m.metallic = in.number();
        m.roughness = in.number();
        m.diffuse = in.string();
        m.normal = in.string();
        data.surfaces[s.material] = m;
        data.geometry.sections.push_back(std::move(s));
    }
    if (next != n)
    {
        throw std::runtime_error("Incomplete shuttle sections");
    }
    in.done();
    return data;
}

void writeTexture(const std::filesystem::path& path, const TextureData& data)
{
    Writer out;
    out.u32(data.width);
    out.u32(data.height);
    out.bytes.insert(out.bytes.end(), data.pixels.begin(), data.pixels.end());
    out.save(path, textureMagic);
}
TextureData readTexture(const std::filesystem::path& path)
{
    Reader in(path, textureMagic);
    TextureData data;
    data.width = in.u32();
    data.height = in.u32();
    if (!data.width || !data.height || data.width > 16384 || data.height > 16384)
    {
        throw std::runtime_error("Invalid texture dimensions");
    }
    const auto size = std::size_t(data.width) * data.height * 4;
    in.need(size);
    const auto header = in.cursor;
    in.cursor += size;
    in.done();
    data.pixels = std::move(in.bytes);
    data.pixels.erase(data.pixels.begin(), data.pixels.begin() + header);
    return data;
}
void writeShaders(const std::filesystem::path& path, const ShaderSources& sources)
{
    Writer out;
    out.u32(static_cast<std::uint32_t>(sources.size()));
    for (const auto& s : sources)
    {
        out.string(s.first);
        out.string(s.second);
    }
    out.save(path, shaderMagic);
}
ShaderSources readShaders(const std::filesystem::path& path)
{
    Reader in(path, shaderMagic);
    ShaderSources sources;
    const auto n = in.u32();
    if (n > 4096)
    {
        throw std::runtime_error("Invalid shader count");
    }
    for (unsigned i = 0; i < n; ++i)
    {
        auto key = in.string();
        auto source = in.string();
        if (!sources.emplace(key, source).second)
        {
            throw std::runtime_error("Duplicate shader");
        }
    }
    in.done();
    return sources;
}
