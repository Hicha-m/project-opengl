#pragma once

#include <vector>

#include <glm/glm.hpp>

struct Sommet
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
    glm::vec3 tangent;
};

class Sphere
{
public:

    Sphere(float radius, unsigned int segments, unsigned int rings);

    void draw() const;

private:

    unsigned int VAO;
    unsigned int VBO;
    unsigned int EBO;

    unsigned int indexCount;

    void generateMesh(
        float radius,
        unsigned int segments,
        unsigned int rings
    );

    void setupMesh(
        const std::vector<Sommet>& vertices,
        const std::vector<unsigned int>& indices
    );
};