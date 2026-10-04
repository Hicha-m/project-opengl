#include <algorithm>
#include <vector>
//-----------------------------------------------------------------------------
// Simple 2D texture class
//-----------------------------------------------------------------------------
#include "graphics/Texture2D.h"
#include "platform/ResourcePaths.h"
#include <iostream>
#include <cassert>
#define STB_IMAGE_IMPLEMENTATION
#define STBI_WINDOWS_UTF8
#include "stb_image.h"

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
Texture2D::Texture2D()
	: mTexture(0)
{
}

//-----------------------------------------------------------------------------
// Destructor
//-----------------------------------------------------------------------------
Texture2D::~Texture2D()
{
	glDeleteTextures(1, &mTexture);
}

//-----------------------------------------------------------------------------
// Load a texture with a given filename using stb image loader
// http://nothings.org/stb_image.h
// Creates mip maps if generateMipMaps is true.
//-----------------------------------------------------------------------------
bool Texture2D::loadTexture(const string& fileName, bool generateMipMaps)
{
	int width, height, components;

	// Use stbi image library to load our image
	const auto bytes = ResourcePaths::read(fileName);
	unsigned char* imageData = bytes.empty() ? nullptr : stbi_load_from_memory(bytes.data(), int(bytes.size()), &width, &height, &components, STBI_rgb_alpha);

	if (imageData == NULL)
	{
		std::cerr << "Error loading texture '" << fileName << "'" << std::endl;
		return false;
	}

	// Invert image
	int widthInBytes = width * 4;
	unsigned char *top = NULL;
	unsigned char *bottom = NULL;
	unsigned char temp = 0;
	int halfHeight = height / 2;
	for (int row = 0; row < halfHeight; row++)
	{
		top = imageData + row * widthInBytes;
		bottom = imageData + (height - row - 1) * widthInBytes;
		for (int col = 0; col < widthInBytes; col++)
		{ 
			temp = *top;
			*top = *bottom;
			*bottom = temp;
			top++;
			bottom++;
		}
	}

	glGenTextures(1, &mTexture);
	glBindTexture(GL_TEXTURE_2D, mTexture); // all upcoming GL_TEXTURE_2D operations will affect our texture object (mTexture)

	// Set the texture wrapping/filtering options (on the currently bound texture object)
	// GL_CLAMP_TO_EDGE
	// GL_REPEAT
	// GL_MIRRORED_REPEAT
	// GL_CLAMP_TO_BORDER
	// GL_LINEAR
	// GL_NEAREST
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);

	if (generateMipMaps)
		glGenerateMipmap(GL_TEXTURE_2D);

	stbi_image_free(imageData);
	glBindTexture(GL_TEXTURE_2D, 0); // unbind texture when done so we don't accidentally mess up our mTexture

	return true;
}

//-----------------------------------------------------------------------------
// Bind the texture unit passed in as the active texture in the shader
//-----------------------------------------------------------------------------
void Texture2D::bind(GLuint texUnit)
{
	assert(texUnit < 32);

	glActiveTexture(GL_TEXTURE0 + texUnit);
	glBindTexture(GL_TEXTURE_2D, mTexture);
}

//-----------------------------------------------------------------------------
// Unbind the texture unit passed in as the active texture in the shader
//-----------------------------------------------------------------------------
void Texture2D::unbind(GLuint texUnit)
{
	glActiveTexture(GL_TEXTURE0 + texUnit);
	glBindTexture(GL_TEXTURE_2D, 0);
}

// Preserve the caller's active unit, binding and pixel packing settings.
bool Texture2D::createRed(int width, int height, const float* pixels, bool floatingPoint)
{
    if (width <= 0 || height <= 0 || !pixels) return false;
    GLint binding, alignment;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &binding);
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment);
    if (!mTexture) glGenTextures(1, &mTexture);
    glBindTexture(GL_TEXTURE_2D, mTexture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    mRedFloatingPoint = floatingPoint;
    std::vector<unsigned char> mask;
    if (!floatingPoint) {
        mask.resize(std::size_t(width) * height);
        for (std::size_t i = 0; i < mask.size(); ++i)
            mask[i] = static_cast<unsigned char>(std::clamp(pixels[i], 0.0f, 1.0f) * 255.0f + 0.5f);
    }
#ifdef PROJECT_MOBILE
    const GLint scalarFormat = GL_R16F; // ES 3.0 guarantees linear sampling of half floats.
#else
    const GLint scalarFormat = GL_R32F;
#endif
    glTexImage2D(GL_TEXTURE_2D, 0, floatingPoint ? scalarFormat : GL_R8, width, height, 0,
        GL_RED, floatingPoint ? GL_FLOAT : GL_UNSIGNED_BYTE, floatingPoint ? static_cast<const void*>(pixels) : mask.data());
    glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
    glBindTexture(GL_TEXTURE_2D, binding);
    mRedWidth = width; mRedHeight = height;
    return true;
}
bool Texture2D::updateRed(int width, int height, const float* pixels)
{
    if (!mTexture || width != mRedWidth || height != mRedHeight || !pixels) return false;
    GLint binding, alignment;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &binding);
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment);
    glBindTexture(GL_TEXTURE_2D, mTexture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    std::vector<unsigned char> mask;
    if (!mRedFloatingPoint) {
        mask.resize(std::size_t(width) * height);
        for (std::size_t i = 0; i < mask.size(); ++i)
            mask[i] = static_cast<unsigned char>(std::clamp(pixels[i], 0.0f, 1.0f) * 255.0f + 0.5f);
    }
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RED,
        mRedFloatingPoint ? GL_FLOAT : GL_UNSIGNED_BYTE, mRedFloatingPoint ? static_cast<const void*>(pixels) : mask.data());
    glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
    glBindTexture(GL_TEXTURE_2D, binding);
    return true;
}
