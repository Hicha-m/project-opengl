#pragma once
#include "assets/Geometry.h"
#include <array>
#include <map>

// CPU-only data shared by the converter, runtime and tests.
struct ShuttleSurface
{
    glm::vec3 color{0.8f};
    float metallic = 0, roughness = 0.7f;
    std::string diffuse, normal;
};
struct ShuttleData
{
    GeometryData geometry; // Normalized once, with contiguous material sections.
    std::map<std::string, ShuttleSurface> surfaces;
};
struct TextureData
{
    unsigned width = 0, height = 0;
    std::vector<unsigned char> pixels; // Bottom row first, RGBA8.
};
using ShaderSources = std::map<std::string, std::string>;
ShuttleData prepareShuttle(const std::filesystem::path& obj, const std::filesystem::path& mtl);
void writeShuttle(const std::filesystem::path& path, const ShuttleData& data);
ShuttleData readShuttle(const std::filesystem::path& path);
void writeTexture(const std::filesystem::path& path, const TextureData& data);
TextureData readTexture(const std::filesystem::path& path);
void writeShaders(const std::filesystem::path& path, const ShaderSources& sources);
ShaderSources readShaders(const std::filesystem::path& path);
