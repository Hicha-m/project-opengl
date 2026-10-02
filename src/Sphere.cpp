

#include "Sphere.h"

#include <GL/glew.h>

#include <cmath>

Sphere::Sphere(
    float radius,
    unsigned int segments,
    unsigned int rings
)
{
    generateMesh(radius, segments, rings);
}

void Sphere::generateMesh(
    float radius,
    unsigned int segments,
    unsigned int rings
)
{
    std::vector<Sommet> vertices;
    std::vector<unsigned int> indices;

    const float PI = 3.14159265359f;

    for (unsigned int y = 0; y <= rings; ++y)
    {
        float v = static_cast<float>(y) / rings;

        float phi = v * PI;

        for (unsigned int x = 0; x <= segments; ++x)
        {
            float u = static_cast<float>(x) / segments;

            float theta = u * 2.0f * PI;

            float sinPhi = std::sin(phi);
            float cosPhi = std::cos(phi);

            float sinTheta = std::sin(theta);
            float cosTheta = std::cos(theta);

            glm::vec3 position;

            position.x = radius * sinPhi * cosTheta;
            position.y = radius * cosPhi;
            position.z = radius * sinPhi * sinTheta;

            glm::vec3 normal =
                glm::normalize(position);

            glm::vec2 uv(u, 1.0f - v);

            vertices.push_back({
                position,
                normal,
                uv,
                glm::vec3(0.0f) // tangent init to zero, will be calculated later
            });
        }
    }


        for (unsigned int y = 0; y < rings; ++y)
    {
        for (unsigned int x = 0; x < segments; ++x)
        {
            unsigned int current =
                y * (segments + 1) + x;

            unsigned int next =
                current + segments + 1;

            indices.push_back(current);
            indices.push_back(next);
            indices.push_back(current + 1);

            indices.push_back(current + 1);
            indices.push_back(next);
            indices.push_back(next + 1);
        }
    }

    // ------------------------------------------------------------
    // Calculate tangent for each vertex
    // ------------------------------------------------------------

    for (size_t i = 0; i < indices.size(); i += 3)
    {
        Sommet& v0 = vertices[indices[i]];
        Sommet& v1 = vertices[indices[i + 1]];
        Sommet& v2 = vertices[indices[i + 2]];

        glm::vec3 edge1 =
            v1.position - v0.position;

        glm::vec3 edge2 =
            v2.position - v0.position;

        glm::vec2 deltaUV1 =
            v1.uv - v0.uv;

        glm::vec2 deltaUV2 =
            v2.uv - v0.uv;

        float determinant =
            deltaUV1.x * deltaUV2.y -
            deltaUV2.x * deltaUV1.y;

        if (std::abs(determinant) < 0.000001f)
            continue;

        float f = 1.0f / determinant;

        glm::vec3 tangent =
            f * (
                deltaUV2.y * edge1 -
                deltaUV1.y * edge2
            );

        v0.tangent += tangent;
        v1.tangent += tangent;
        v2.tangent += tangent;
    }

    // ------------------------------------------------------------
    // Normalize tangents and make them orthogonal to the normal
    // ------------------------------------------------------------

    for (Sommet& vertex : vertices)
    {
        vertex.tangent =
            glm::normalize(
                vertex.tangent -
                vertex.normal *
                glm::dot(vertex.normal, vertex.tangent)
            );
    }

    setupMesh(vertices, indices);
}

void Sphere::setupMesh(
    const std::vector<Sommet>& vertices,
    const std::vector<unsigned int>& indices
)
{
    indexCount = indices.size();

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(
        GL_ARRAY_BUFFER,
        vertices.size() * sizeof(Sommet),
        vertices.data(),
        GL_STATIC_DRAW
    );

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        indices.size() * sizeof(unsigned int),
        indices.data(),
        GL_STATIC_DRAW
    );

    // Position
    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Sommet),
        (void*)offsetof(Sommet, position)
    );

    glEnableVertexAttribArray(0);

    // Normal
    glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Sommet),
        (void*)offsetof(Sommet, normal)
    );

    glEnableVertexAttribArray(1);

    // UV
    glVertexAttribPointer(
        2,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Sommet),
        (void*)offsetof(Sommet, uv)
    );

    glEnableVertexAttribArray(2);

    // Tangent
    glVertexAttribPointer(
        3,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Sommet),
        (void*)offsetof(Sommet, tangent)
    );

    glEnableVertexAttribArray(3);

    glBindVertexArray(0); // to reset the VAO state
}


void Sphere::draw() const
{
    glBindVertexArray(VAO);

    glDrawElements(
        GL_TRIANGLES,
        indexCount,
        GL_UNSIGNED_INT,
        nullptr
    );

    glBindVertexArray(0);
}