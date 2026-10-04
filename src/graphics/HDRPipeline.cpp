#include "graphics/HDRPipeline.h"
#include <algorithm>
namespace {
    bool load(ShaderProgram& shader, const char* fragment) {
        if (!shader.loadShaders("shaders/post.vert",fragment)) return false;
        GLint linked; glGetProgramiv(shader.getProgram(),GL_LINK_STATUS,&linked);
        return linked == GL_TRUE;
    }
    void texture(GLuint& id, int width, int height) {
        glGenTextures(1,&id); glBindTexture(GL_TEXTURE_2D,id);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA16F,width,height,0,GL_RGBA,GL_FLOAT,nullptr);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    }
}
HDRPipeline::~HDRPipeline() { release(); }
void HDRPipeline::release()
{
    if (mVAO) glDeleteVertexArrays(1,&mVAO);
    if (mSceneFBO) glDeleteFramebuffers(1,&mSceneFBO);
    if (mDepth) glDeleteRenderbuffers(1,&mDepth);
    if (mScene) glDeleteTextures(1,&mScene);
    if (mEmission) glDeleteTextures(1,&mEmission);
    glDeleteFramebuffers(2,mFBO); glDeleteTextures(2,mTexture);
    mVAO=mSceneFBO=mDepth=mScene=mEmission=0; mFBO[0]=mFBO[1]=mTexture[0]=mTexture[1]=0;
    mWidth=mHeight=0;
}
bool HDRPipeline::targets(int width, int height)
{
    if (width <= 0 || height <= 0) return false;
    if (width == mWidth && height == mHeight) return true;
    mWidth = mHeight = 0;
    if (mSceneFBO) glDeleteFramebuffers(1,&mSceneFBO);
    if (mDepth) glDeleteRenderbuffers(1,&mDepth);
    if (mScene) glDeleteTextures(1,&mScene);
    if (mEmission) glDeleteTextures(1,&mEmission);
    glDeleteFramebuffers(2,mFBO); glDeleteTextures(2,mTexture);
    GLint binding, tex, renderbuffer;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING,&binding); glGetIntegerv(GL_TEXTURE_BINDING_2D,&tex);
    glGetIntegerv(GL_RENDERBUFFER_BINDING,&renderbuffer);
    texture(mScene,width,height);
    texture(mEmission,width,height);
    glGenFramebuffers(1,&mSceneFBO); glBindFramebuffer(GL_FRAMEBUFFER,mSceneFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,mScene,0);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT1,GL_TEXTURE_2D,mEmission,0);
    const GLenum attachments[]={GL_COLOR_ATTACHMENT0,GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2,attachments);
    glGenRenderbuffers(1,&mDepth); glBindRenderbuffer(GL_RENDERBUFFER,mDepth);
    glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH_COMPONENT24,width,height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,mDepth);
    bool complete = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    glGenFramebuffers(2,mFBO);
    for (int i=0;i<2;++i) {
        texture(mTexture[i],std::max(1,width/2),std::max(1,height/2));
        glBindFramebuffer(GL_FRAMEBUFFER,mFBO[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,mTexture[i],0);
        complete = complete && glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    }
    glBindFramebuffer(GL_FRAMEBUFFER,binding); glBindTexture(GL_TEXTURE_2D,tex);
    glBindRenderbuffer(GL_RENDERBUFFER,renderbuffer);
    if (complete) { mWidth=width; mHeight=height; }
    return complete;
}
bool HDRPipeline::init(int width, int height)
{
    if (!mVAO) {
        if (!load(mBright,"shaders/bright.frag") || !load(mBlur,"shaders/blur.frag")
            || !load(mComposite,"shaders/composite.frag")) return false;
        glGenVertexArrays(1,&mVAO);
    }
    return targets(width,height);
}
bool HDRPipeline::begin(int width, int height)
{
    if (!mVAO || !targets(width,height)) return false;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING,&mDestination); glGetIntegerv(GL_VIEWPORT,mViewport);
    glBindFramebuffer(GL_FRAMEBUFFER,mSceneFBO); glViewport(0,0,width,height);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    const GLfloat zero[]={0,0,0,0};
    glClearBufferfv(GL_COLOR,1,zero);
    return true;
}
void HDRPipeline::finish(bool bloom)
{
    GLint program, vao, active, binding[2], polygon[2];
    glGetIntegerv(GL_CURRENT_PROGRAM,&program); glGetIntegerv(GL_VERTEX_ARRAY_BINDING,&vao);
    glGetIntegerv(GL_ACTIVE_TEXTURE,&active); glGetIntegerv(GL_POLYGON_MODE,polygon);
    for(int i=0;i<2;++i) { glActiveTexture(GL_TEXTURE0+i); glGetIntegerv(GL_TEXTURE_BINDING_2D,&binding[i]); }
    const bool depth=glIsEnabled(GL_DEPTH_TEST), blend=glIsEnabled(GL_BLEND), cull=glIsEnabled(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST); glDisable(GL_BLEND); glDisable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK,GL_FILL); glBindVertexArray(mVAO);
    glViewport(0,0,std::max(1,mWidth/2),std::max(1,mHeight/2));
    glBindFramebuffer(GL_FRAMEBUFFER,mFBO[0]); mBright.use();
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D,mEmission); mBright.setUniform("emissionSource",1);
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D,mScene); mBright.setUniform("source",0);
    glDrawArrays(GL_TRIANGLES,0,3);
    for(int pass=0;pass<8;++pass) {
        int target=(pass+1)%2, source=pass%2;
        glBindFramebuffer(GL_FRAMEBUFFER,mFBO[target]); mBlur.use();
        glBindTexture(GL_TEXTURE_2D,mTexture[source]); mBlur.setUniform("source",0);
        mBlur.setUniform("axis",glm::vec2(pass%2 ? 0.0f : 1.0f,pass%2 ? 1.0f : 0.0f));
        glDrawArrays(GL_TRIANGLES,0,3);
    }
    glBindFramebuffer(GL_FRAMEBUFFER,mDestination); glViewport(mViewport[0],mViewport[1],mViewport[2],mViewport[3]);
    mComposite.use(); glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D,mScene);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D,mTexture[0]);
    mComposite.setUniform("scene",0); mComposite.setUniform("bloom",1); mComposite.setUniform("bloomEnabled",int(bloom));
    glDrawArrays(GL_TRIANGLES,0,3);
    for(int i=0;i<2;++i) { glActiveTexture(GL_TEXTURE0+i); glBindTexture(GL_TEXTURE_2D,binding[i]); }
    glActiveTexture(active); glUseProgram(program); glBindVertexArray(vao);
    glPolygonMode(GL_FRONT_AND_BACK,polygon[0]);
    if(depth) glEnable(GL_DEPTH_TEST);
    if(blend) glEnable(GL_BLEND);
    if(cull) glEnable(GL_CULL_FACE);
}
