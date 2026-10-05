#ifndef MESH_H
#define MESH_H

#include <vector>
#include <string>

#include <glad/gl.h>
#include "glm/glm.hpp"


#include "assets/Geometry.h"

class Mesh
{
public:

	 Mesh();
	~Mesh();

	bool loadOBJ(const std::string& filename);
	void draw();
    const std::vector<Vertex>& vertices() const { return mVertices; }
    const std::vector<MeshSection>& sections() const { return mSections; }

	// Permet à Sphere de construire directement un Mesh
    void setVertices(
        const std::vector<Vertex>& vertices
    );

private:

	void initBuffers();

	bool mLoaded;
	std::vector<Vertex> mVertices;
    std::vector<MeshSection> mSections;
	GLuint mVBO, mVAO;
};

#endif // MESH_H
