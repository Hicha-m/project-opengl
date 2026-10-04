#pragma once

#include "graphics/Mesh.h"

class Sphere
{
public:

    Sphere(
        float radius,
        unsigned int segments,
        unsigned int rings
    );

    Mesh& getMesh();
    float getRadius() const { return mRadius; }

private:

    Mesh mMesh;
    float mRadius;

    void generateMesh(
        float radius,
        unsigned int segments,
        unsigned int rings
    );
};
