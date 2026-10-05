//-----------------------------------------------------------------------------
// Basic Mesh class
//-----------------------------------------------------------------------------
#include "graphics/Mesh.h"
#include "platform/ResourcePaths.h"
#include <iostream>
#include <stdexcept>

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
Mesh::Mesh() : mLoaded(false), mVBO(0), mVAO(0)
{
}
//-----------------------------------------------------------------------------
// Destructor
//-----------------------------------------------------------------------------
Mesh::~Mesh()
{
    if (mVAO != 0)
    {
        glDeleteVertexArrays(1, &mVAO);
    }
    if (mVBO != 0)
    {
        glDeleteBuffers(1, &mVBO);
    }
}

// OBJ geometry, triangulation and named material sections. MTL assets remain
// owned by the scene. Parse into temporary data: failure preserves a loaded mesh.
bool Mesh::loadOBJ(const std::string& filename)
{
    try
    {
        auto data = readOBJ(ResourcePaths::resolve(filename));
        setVertices(data.vertices);
        mSections = std::move(data.sections);
        return true;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return false;
    }
}

//-----------------------------------------------------------------------------
// Create and initialize the vertex buffer and vertex array object
// Must have valid, non-empty std::vector of Vertex objects.
//-----------------------------------------------------------------------------
void Mesh::initBuffers()
{

    if (mVAO != 0)
    {
        glDeleteVertexArrays(1, &mVAO);
    }
    if (mVBO != 0)
    {
        glDeleteBuffers(1, &mVBO);
    }

    // idem pour normal, texCoords, tangent
    glGenVertexArrays(1, &mVAO);
    glGenBuffers(1, &mVBO);

    glBindVertexArray(mVAO);
    glBindBuffer(GL_ARRAY_BUFFER, mVBO);
    glBufferData(GL_ARRAY_BUFFER, mVertices.size() * sizeof(Vertex), &mVertices[0], GL_STATIC_DRAW);

    // Vertex Positions
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    // Normals attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    // Vertex Texture Coords
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)(6 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);

    // Vertex Tangent
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)(8 * sizeof(GLfloat)));
    glEnableVertexAttribArray(3);

    // unbind to make sure other code does not change it somewhere else
    glBindVertexArray(0);
}

void Mesh::setVertices(const std::vector<Vertex>& vertices)
{
    mVertices = vertices;
    mSections.clear();

    if (mVertices.empty())
    {
        mLoaded = false;
        return;
    }

    initBuffers();

    mLoaded = true;
}

//-----------------------------------------------------------------------------
// Render the mesh
//-----------------------------------------------------------------------------
void Mesh::draw()
{
    if (!mLoaded)
    {
        return;
    }

    glBindVertexArray(mVAO);
    glDrawArrays(GL_TRIANGLES, 0, mVertices.size());
    glBindVertexArray(0);
}
