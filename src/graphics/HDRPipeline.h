#pragma once
#include "graphics/ShaderProgram.h"

// RGBA16F scene -> bright extraction -> separable blur -> tone mapped composite.
// Owns GL resources; init/release/destruction require a current context.
class HDRPipeline
{
public:
    HDRPipeline() = default;
    ~HDRPipeline();
    HDRPipeline(const HDRPipeline&) = delete;
    HDRPipeline& operator=(const HDRPipeline&) = delete;
    bool init(int width, int height);
    bool begin(int width, int height);
    void finish(bool bloom = true);
    void release();
private:
    bool targets(int width, int height);
    ShaderProgram mBright, mBlur, mComposite;
    GLuint mVAO = 0, mSceneFBO = 0, mScene = 0, mEmission = 0, mDepth = 0;
    GLuint mFBO[2]{}, mTexture[2]{};
    int mWidth = 0, mHeight = 0;
    GLint mDestination = 0, mViewport[4]{};
};
