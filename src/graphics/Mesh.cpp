//-----------------------------------------------------------------------------
// Basic Mesh class
//-----------------------------------------------------------------------------
#include "graphics/Mesh.h"
#include "platform/ResourcePaths.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <cmath>
#include <stdexcept>


//-----------------------------------------------------------------------------
// split
//
// Params:  s - string to split
//		    t - string to split (ie. delimiter)
//
//Result:  Splits string according to some substring and returns it as a vector.
//-----------------------------------------------------------------------------
std::vector<std::string> split(std::string s, std::string t)
{
	std::vector<std::string> res;
	while(1)
	{
		size_t pos = s.find(t);
		if(pos == std::string::npos)
		{
			res.push_back(s);
			break;
		}
		res.push_back(s.substr(0, pos));
		s = s.substr(pos + t.length());;
	}
	return res;
}


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
Mesh::Mesh()
    : mLoaded(false), mVBO(0), mVAO(0)
{
}
//-----------------------------------------------------------------------------
// Destructor
//-----------------------------------------------------------------------------
Mesh::~Mesh()
{
if (mVAO != 0) glDeleteVertexArrays(1, &mVAO);
if (mVBO != 0) glDeleteBuffers(1, &mVBO);
}

// OBJ geometry, triangulation and named material sections. MTL assets remain
// owned by the scene. Parse into temporary data: failure preserves a loaded mesh.
bool Mesh::loadOBJ(const std::string& filename)
{
    const auto bytes = ResourcePaths::read(filename);
    std::istringstream input(std::string(bytes.begin(), bytes.end()));
    if(bytes.empty()) { std::cerr<<"Cannot open "<<filename<<"\n"; return false; }
    std::vector<glm::vec3> positions,normals;
    std::vector<glm::vec2> uvs;
    std::vector<Vertex> vertices;
    std::vector<MeshSection> sections;
    std::string line, material="default";
    auto index=[](const std::string& token,std::size_t count)->int {
        std::size_t used=0;
        const int value=std::stoi(token,&used);
        if(used!=token.size() || !value) throw std::runtime_error("Invalid OBJ index");
        const long resolved=value>0 ? long(value)-1 : long(count)+value;
        if(resolved<0 || resolved>=long(count)) throw std::runtime_error("OBJ index out of range");
        return int(resolved);
    };
    try {
        while(std::getline(input,line)) {
            std::istringstream row(line); std::string command; row>>command;
            if(command=="v" || command=="vn") {
                glm::vec3 v; if(!(row>>v.x>>v.y>>v.z)) return false;
                if(!std::isfinite(v.x) || !std::isfinite(v.y) || !std::isfinite(v.z)) return false;
                if(command=="v") positions.push_back(v);
                else normals.push_back(glm::length(v)>0 ? glm::normalize(v) : glm::vec3(0));
            } else if(command=="vt") {
                glm::vec2 uv(0); if(!(row>>uv.x)) return false; row>>uv.y;
                if(!std::isfinite(uv.x) || !std::isfinite(uv.y)) return false;
                uvs.push_back(uv);
            } else if(command=="usemtl") row>>material;
            else if(command=="f") {
                std::vector<Vertex> polygon; std::string token;
                while(row>>token) {
                    if(token[0]=='#') break;
                    const auto data=split(token,"/");
                    if(data.empty() || data.size()>3) return false;
                    Vertex vertex{}; vertex.position=positions.at(index(data[0],positions.size()));
                    if(data.size()>1 && !data[1].empty()) vertex.texCoords=uvs.at(index(data[1],uvs.size()));
                    if(data.size()>2 && !data[2].empty()) vertex.normal=normals.at(index(data[2],normals.size()));
                    polygon.push_back(vertex);
                }
                if(polygon.size()<3) return false;
                for(std::size_t i=1;i+1<polygon.size();++i) {
                    Vertex triangle[]={polygon[0],polygon[i],polygon[i+1]};
                    const auto edge1=triangle[1].position-triangle[0].position;
                    const auto edge2=triangle[2].position-triangle[0].position;
                    const auto cross=glm::cross(edge1,edge2);
                    const auto normal=glm::length(cross)>0 ? glm::normalize(cross) : glm::vec3(0,1,0);
                    const auto uv1=triangle[1].texCoords-triangle[0].texCoords;
                    const auto uv2=triangle[2].texCoords-triangle[0].texCoords;
                    const float determinant=uv1.x*uv2.y-uv2.x*uv1.y;
                    const auto tangent=std::abs(determinant)>0.000001f ? (edge1*uv2.y-edge2*uv1.y)/determinant : glm::vec3(0);
                    if(sections.empty() || sections.back().material!=material)
                        sections.push_back({material,vertices.size(),0});
                    for(auto& vertex:triangle) {
                        if(glm::length(vertex.normal)==0) vertex.normal=normal;
                        vertex.tangent=glm::length(tangent)>0 ? glm::normalize(tangent) : glm::vec3(0);
                        vertices.push_back(vertex); ++sections.back().count;
                    }
                }
            }
        }
    } catch(const std::exception& error) {
        std::cerr<<"Invalid OBJ "<<filename<<": "<<error.what()<<"\n"; return false;
    }
    if(vertices.empty()) return false;
    setVertices(vertices); mSections=std::move(sections);
    return true;
}

//-----------------------------------------------------------------------------
// Create and initialize the vertex buffer and vertex array object
// Must have valid, non-empty std::vector of Vertex objects.
//-----------------------------------------------------------------------------
void Mesh::initBuffers()
{

	if (mVAO != 0) glDeleteVertexArrays(1, &mVAO);
	if (mVBO != 0) glDeleteBuffers(1, &mVBO);


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

void Mesh::setVertices(
    const std::vector<Vertex>& vertices
)
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
	if (!mLoaded) return;

	glBindVertexArray(mVAO);
	glDrawArrays(GL_TRIANGLES, 0, mVertices.size());
	glBindVertexArray(0);
}
