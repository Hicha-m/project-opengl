#pragma once

#include "Mesh.h"

class Sphere
{
public:

    Sphere(
        float radius,
        unsigned int segments,
        unsigned int rings
    );

    Mesh& getMesh();

private:

    Mesh mMesh;

    void generateMesh(
        float radius,
        unsigned int segments,
        unsigned int rings
    );
};