#pragma once
#include <filesystem>
#include <glm/glm.hpp>
#include <string>
#include <vector>

struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
    glm::vec3 tangent;
};

struct MeshSection
{
    std::string material;
    std::size_t first = 0, count = 0;
};

struct GeometryData
{
    std::vector<Vertex> vertices;
    std::vector<MeshSection> sections;
};
GeometryData readOBJ(const std::filesystem::path& filename);
