//-----------------------------------------------------------------------------
// Simple 2D texture class
//-----------------------------------------------------------------------------
#ifndef TEXTURE2D_H
#define TEXTURE2D_H

#include "GL/glew.h"
#include <string>
using std::string;

class Texture2D
{
public:
	Texture2D();
	virtual ~Texture2D();

	bool loadTexture(const string& fileName, bool generateMipMaps = true);
	// Dynamic single-channel mask, no mipmaps. S repeats; T clamps at poles.
	bool createRed(int width, int height, const float* pixels);
	bool updateRed(int width, int height, const float* pixels);
	void bind(GLuint texUnit = 0);
	void unbind(GLuint texUnit = 0);

private:
 	// supprime la copie (ressource OpenGL unique)
	Texture2D(const Texture2D&)            = delete;
	Texture2D& operator=(const Texture2D&) = delete;
	// move autorisé
	Texture2D(Texture2D&&) noexcept;
	Texture2D& operator=(Texture2D&&) noexcept;

	GLuint mTexture;
	int mRedWidth = 0, mRedHeight = 0;
};
#endif //TEXTURE2D_H
