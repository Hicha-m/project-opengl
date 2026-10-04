#pragma once
#include <memory>
#include <cstddef>
#include "scene/SceneResources.h"
#include "scene/Scene.h"
#include "scene/LightManager.h"
#include "graphics/Renderer.h"
#include "animation/Timeline.h"
#include "camera/CinematicCamera.h"
#include "systems/MeteorSystem.h"
#include "systems/MeteorShower.h"
#include "systems/ImpactLightSystem.h"
#include "systems/ImpactParticleEmitter.h"
#include "systems/MeteorTrailEmitter.h"

struct GLFWwindow;

struct ApplicationOptions
{
    int width = 1024;
    int height = 768;
    bool fullscreen = true;
    bool visible = true;
};

class Application
{
public:
    explicit Application(ApplicationOptions options = {});
    ~Application();
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    bool init();
    void run(std::size_t frameLimit = 0); // zero: run until window closes
    void shutdown();
    void restartSequence();
    MeteorSystem& meteors() { return mMeteorSystem; }
    ParticleSystem& particles() { return mParticleSystem; }
    const ImpactLightSystem& impactLights() const { return mImpactLightSystem; }

private:
    bool initOpenGL();
    void update(float deltaTime);
    void updateInput(float deltaTime);
    void render();
    void showFPS(double currentTime);
    void onKey(int key, int action);
    void onFramebufferSize(int width, int height);
    static void keyCallback(GLFWwindow*, int, int, int, int);
    static void framebufferCallback(GLFWwindow*, int, int);

    ApplicationOptions mOptions;
    GLFWwindow* mWindow = nullptr;
    bool mGLFWInitialized = false;
    bool mInitialized = false;
    bool mWireframe = false;
    bool mCameraDebug = false;
    bool mFPSMode = false;
    float mMoveSpeed = 5.0f;
    double mFPSStart = 0;
    unsigned mFrameCount = 0;
    double mDebugTimer = 0;

    // Reset explicitly in shutdown while the context is still current.
    std::unique_ptr<SceneResources> mResources;
    Scene mScene;
    Renderer mRenderer;
    LightManager mLightManager;
    FPSCamera mFPSCamera;
    CinematicCamera mCinematicCamera;
    Timeline mTimeline;
    MeteorSystem mMeteorSystem;
    ImpactLightSystem mImpactLightSystem;
    ParticleSystem mParticleSystem;
    ImpactParticleEmitter mImpactParticleEmitter{mParticleSystem};
    MeteorTrailEmitter mMeteorTrailEmitter{mParticleSystem};
    MeteorShower mMeteorShower{mMeteorSystem};
};
