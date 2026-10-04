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
#include "systems/EarthDamageSystem.h"
#include "systems/EarthBreakupSystem.h"
#include "systems/SolarSystem.h"
#include "audio/MusicPlayer.h"
#include "graphics/HDRPipeline.h"

struct GLFWwindow;

struct ApplicationOptions
{
    int width = 1280;
    int height = 720;
    bool fullscreen = false;
    bool visible = true;
    bool music = true;
    bool softwareContext = false; // Headless OSMesa verification, no desktop GPU required.
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
    void seekSequence(float seconds);
    void setPlaybackRate(float rate);
    void toggleSequencePause();
    float sequenceTime() const { return mTimeline.getTime(); }
    float playbackRate() const { return mPlaybackRate; }
    bool sequencePaused() const { return mSequencePaused; }
    bool audioReady() const { return mMusic.ready() && mMusic.impactReady(); }
    MeteorSystem& meteors() { return mMeteorSystem; }
    ParticleSystem& particles() { return mParticleSystem; }
    const EarthBreakupSystem& earthBreakup() const { return mEarthBreakupSystem; }
    const EarthDamageSystem& earthDamage() const { return mEarthDamageSystem; }
    const ImpactLightSystem& impactLights() const { return mImpactLightSystem; }

private:
    bool initOpenGL();
    void update(float deltaTime);
    void simulate(float deltaTime, bool audible);
    void resetSequenceState();
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
    bool mMusicMuted = false;
    MusicPlayer mMusic;
    float mPlaybackRate=1;
    bool mSequencePaused=false;
    bool mClockResync=false;
    float mMoveSpeed = 5.0f;
    double mFPSStart = 0;
    unsigned mFrameCount = 0;
    double mDebugTimer = 0;

    // Reset explicitly in shutdown while the context is still current.
    std::unique_ptr<SceneResources> mResources;
    std::unique_ptr<HDRPipeline> mHDR;
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
    EarthDamageSystem mEarthDamageSystem;
    EarthBreakupSystem mEarthBreakupSystem;
    MeteorShower mMeteorShower{mMeteorSystem};
    SolarSystem mSolarSystem;
};
