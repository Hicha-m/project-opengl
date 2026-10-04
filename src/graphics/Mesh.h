#ifndef MESH_H
#define MESH_H

#include <vector>
#include <string>

#define GLEW_STATIC
#include "GL/glew.h"
#include "glm/glm.hpp"


struct Vertex
{
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texCoords;
    glm::vec3 tangent;
};

struct MeshSection {
    std::string material;
    std::size_t first=0, count=0;
};

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