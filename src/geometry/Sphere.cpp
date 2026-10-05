#include "geometry/Sphere.h"

#include <cmath>

Sphere::Sphere(float radius, unsigned int segments, unsigned int rings) : mRadius(radius)
{
    generateMesh(radius, segments, rings);
}

Mesh& Sphere::getMesh()
{
    return mMesh;
}

void Sphere::generateMesh(float radius, unsigned int segments, unsigned int rings)
{
    std::vector<Vertex> vertices;
    const float PI = 3.14159265359f;
    for (unsigned int y = 0; y <= rings; ++y)
    {
        float v = static_cast<float>(y) / static_cast<float>(rings);
        float phi = v * PI;
        for (unsigned int x = 0; x <= segments; ++x)
        {
            float u = static_cast<float>(x) / static_cast<float>(segments);
            float theta = u * 2.0f * PI;
            float sinPhi = std::sin(phi);
            float cosPhi = std::cos(phi);
            float sinTheta = std::sin(theta);
            float cosTheta = std::cos(theta);
            Vertex vertex{};
            vertex.position =
                glm::vec3(radius * sinPhi * cosTheta, radius * cosPhi, radius * sinPhi * sinTheta);
            vertex.normal = glm::normalize(vertex.position);
            vertex.texCoords = glm::vec2(u, 1.0f - v);
            vertex.tangent = glm::vec3(0.0f);
            vertices.push_back(vertex);
        }
    }

    // Indices -> triangles
    std::vector<unsigned int> indices;
    for (unsigned int y = 0; y < rings; ++y)
    {
        for (unsigned int x = 0; x < segments; ++x)
        {
            unsigned int current = y * (segments + 1) + x;
            unsigned int next = current + segments + 1;
            indices.push_back(current);
            indices.push_back(next);
            indices.push_back(current + 1);
            indices.push_back(current + 1);
            indices.push_back(next);
            indices.push_back(next + 1);
        }
    }

    // Tangents
    for (size_t i = 0; i + 2 < indices.size(); i += 3)
    {
        Vertex& v0 = vertices[indices[i]];
        Vertex& v1 = vertices[indices[i + 1]];
        Vertex& v2 = vertices[indices[i + 2]];
        glm::vec3 edge1 = v1.position - v0.position;
        glm::vec3 edge2 = v2.position - v0.position;
        glm::vec2 deltaUV1 = v1.texCoords - v0.texCoords;
        glm::vec2 deltaUV2 = v2.texCoords - v0.texCoords;
        float determinant = deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y;
        if (std::abs(determinant) < 0.000001f)
        {
            continue;
        }

        float f = 1.0f / determinant;
        glm::vec3 tangent = f * (deltaUV2.y * edge1 - deltaUV1.y * edge2);
        v0.tangent += tangent;
        v1.tangent += tangent;
        v2.tangent += tangent;
    }

    // Normalize
    for (Vertex& vertex : vertices)
    {
        vertex.tangent = vertex.tangent - vertex.normal * glm::dot(vertex.normal, vertex.tangent);
        if (glm::length(vertex.tangent) > 0.0001f)
        {
            vertex.tangent = glm::normalize(vertex.tangent);
        }
    }

    // Convert indexed sphere into triangles
    std::vector<Vertex> triangleVertices;
    triangleVertices.reserve(indices.size());
    for (unsigned int index : indices)
    {
        triangleVertices.push_back(vertices[index]);
    }

    mMesh.setVertices(triangleVertices);
}
