#include "Application.h"
#include "platform/Window.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include "scene/SceneSetup.h"
#include "cinematic/MainSequence.h"

namespace
{
    constexpr const char* APP_TITLE = "Project - Space";
    constexpr float MOUSE_SENSITIVITY = 0.1f;
    constexpr float MAX_DISTANCE = 100000000.0f;
}

Application::Application(ApplicationOptions options)
    : mOptions(options),
      mFPSCamera(glm::vec3(0, 3.5f, 10), glm::radians(110.0f), glm::radians(55.0f))
{
}

Application::~Application()
{
    shutdown();
}

bool Application::init()
{
    if (mInitialized) return true;
    if (!initOpenGL())
    {
        shutdown();
        return false;
    }
    mResources = std::make_unique<SceneResources>();
    mHDR = std::make_unique<HDRPipeline>();
    if (!SceneSetup::build(mScene, mLightManager, *mResources)
        || !mEarthBreakupSystem.initGraphics(mResources->earthSphere.getMesh())
        || !mHDR->init(mOptions.width,mOptions.height)
        || !mMeteorSystem.initGraphics()
        || !mParticleSystem.initGraphics()
        || !MainSequence::build(mTimeline, mCinematicCamera, mScene, mMeteorShower, &mSolarSystem, &mEarthBreakupSystem))
    {
        std::cerr << "Scene initialization failed\n";
        shutdown();
        return false;
    }
    if(mOptions.music && mMusic.load("build/music/cinematic.wav"))
    {
        mTimeline.setDuration(mMusic.duration());
        if(!mMusic.loadImpact("build/music/impact.wav")) std::cerr<<"Impact sound unavailable\n";
    }
    else if (mOptions.music) std::cerr << "Music unavailable; continuing without audio\n";
    WindowSystem::setAudioDevice(mWindow, mMusic.device());
    mFPSStart = WindowSystem::time();
    mInitialized = true;
    return true;
}

bool Application::initOpenGL()
{
    if (mOptions.width <= 0 || mOptions.height <= 0) return false;
    mWindow = WindowSystem::create(mOptions.width, mOptions.height, APP_TITLE,
        mOptions.visible, mOptions.fullscreen, mOptions.softwareContext);
    if (!mWindow || !WindowSystem::loadGraphics()) return false;
    int width = mOptions.width, height = mOptions.height;
    WindowSystem::size(mWindow, &width, &height);
    WindowSystem::setUserPointer(mWindow, this);
    WindowSystem::setKeyCallback(mWindow, keyCallback);
    WindowSystem::setFramebufferCallback(mWindow, framebufferCallback);
    WindowSystem::setCursor(mWindow, width / 2.0, height / 2.0);
    WindowSystem::framebufferSize(mWindow, &width, &height);
    onFramebufferSize(width, height);
    glClearColor(0.06f, 0.06f, 0.07f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    return true;
}

void Application::run(std::size_t frameLimit, bool present)
{
    if (!mInitialized) return;
    if(mMusic.ready() && !mMusic.running()) mMusic.restart();
    double lastTime = WindowSystem::time();
    std::size_t frames = 0;
    while (!WindowSystem::shouldClose(mWindow))
    {
        WindowSystem::pollEvents();
        if (WindowSystem::shouldClose(mWindow)) break;
        if (WindowSystem::suspended(mWindow)) {
            mMusic.setPaused(true); mWasSuspended = true;
            lastTime = WindowSystem::time(); WindowSystem::wait(); continue;
        }
        if (mWasSuspended || WindowSystem::consumeResume(mWindow)) {
            mMusic.setPaused(mSequencePaused); mClockResync = true; mWasSuspended = false;
#ifdef PROJECT_MOBILE
            std::cout << "[MOBILE] foreground resumed timeline=" << mTimeline.getTime() << " paused=" << mSequencePaused << std::endl;
#endif
        }
        double currentTime = WindowSystem::time();
        if(mClockResync) { lastTime=currentTime; mClockResync=false; }
        update(static_cast<float>(currentTime - lastTime));
        render();
        showFPS(currentTime);
        if (present) WindowSystem::swap(mWindow);
        lastTime = currentTime;
        if (frameLimit && ++frames >= frameLimit) break;
    }
}

void Application::update(float deltaTime)
{
    updateInput(deltaTime);
    if(mSequencePaused || !std::isfinite(deltaTime) || deltaTime<=0) return;
    const bool finished=mTimeline.getTime()>=mTimeline.getDuration();
    deltaTime*=mPlaybackRate;
    if(mMusic.running() && !finished) deltaTime=std::max(0.0f,mMusic.position()-mTimeline.getTime());
    if(finished) MainSequence::continueEscape(mScene,mCinematicCamera,deltaTime);
    // Bound physics steps even at x8 or after a slow rendering frame.
    while(deltaTime>0) {
        const float step=std::min(deltaTime,1.0f/60);
        simulate(step,true); deltaTime-=step;
    }
    mEarthDamageSystem.upload(mResources->earthDamageTexture,mResources->earthHeatTexture);
}

void Application::simulate(float deltaTime,bool audible)
{
    mTimeline.update(deltaTime);
    auto sunlight=mLightManager.getDirectionalLight();
    sunlight.direction=glm::normalize(mScene.findObject("Earth")->transform.position-SolarSystem::SunCenter);
    mLightManager.setDirectionalLight(sunlight);
    mMeteorTrailEmitter.observe(mMeteorSystem.meteors());
    const bool hittingCore = mEarthBreakupSystem.active();
    if (hittingCore) mMeteorSystem.update(deltaTime, mEarthBreakupSystem.coreCollider());
    else mMeteorSystem.update(deltaTime, SceneSetup::earthCollider(mScene, *mResources));
    // Advance existing particles before births, so every burst is visible at age zero.
    mParticleSystem.update(deltaTime);
    mMeteorTrailEmitter.update(mMeteorSystem.meteors(), deltaTime);
    const auto* earth = mScene.findObject("Earth");
    mEarthDamageSystem.update(deltaTime);
    if (!hittingCore)
        mEarthDamageSystem.consume(mMeteorSystem.impacts(), earth->transform, mResources->earthSphere.getRadius());
    mEarthBreakupSystem.update(deltaTime,mEarthDamageSystem.destructionLevel(),earth->transform);
    // Core contact only absorbs meteors; Earth effects belong to the intact planet.
    if (!hittingCore) {
        mImpactParticleEmitter.consume(mMeteorSystem.impacts());
        mImpactLightSystem.consume(mMeteorSystem.impacts());
        if(audible) mMusic.playImpacts(mMeteorSystem.impacts(),mFPSMode?mFPSCamera.getPosition():mCinematicCamera.getPosition(),mTimeline.getTime());
    }
    mImpactLightSystem.update(deltaTime);
    mImpactLightSystem.publish(mLightManager);
    mEarthBreakupSystem.publish(mLightManager);
    // No invisible Earth collider or new bombardment after the physical rupture.
    if (mEarthBreakupSystem.active()) mMeteorShower.stop();
    mMeteorShower.update(deltaTime);
    mDebugTimer += deltaTime;
    if (audible && mDebugTimer >= 1.0)
    {
        mDebugTimer = 0;
        const auto position = mCinematicCamera.getPosition();
        std::cout << "[DEBUG] timeline=" << mTimeline.getTime()
            << " playing=" << mTimeline.isPlaying() << " cam=("
            << position.x << ", " << position.y << ", " << position.z << ")\n";
    }
}

void Application::updateInput(float deltaTime)
{
    int width, height;
    WindowSystem::size(mWindow, &width, &height);
    double mouseX, mouseY;
    WindowSystem::cursor(mWindow, &mouseX, &mouseY);
    WindowSystem::setCursor(mWindow, width / 2.0, height / 2.0);
    if (!mFPSMode) return;
    mFPSCamera.rotate(static_cast<float>(width / 2.0 - mouseX) * MOUSE_SENSITIVITY,
                      static_cast<float>(height / 2.0 - mouseY) * MOUSE_SENSITIVITY);
    const float step = mMoveSpeed * deltaTime;
    if (WindowSystem::key(mWindow, WindowSystem::Key::W) == WindowSystem::Press) mFPSCamera.move(step * mFPSCamera.getLook());
    else if (WindowSystem::key(mWindow, WindowSystem::Key::S) == WindowSystem::Press) mFPSCamera.move(-step * mFPSCamera.getLook());
    if (WindowSystem::key(mWindow, WindowSystem::Key::A) == WindowSystem::Press) mFPSCamera.move(-step * mFPSCamera.getRight());
    else if (WindowSystem::key(mWindow, WindowSystem::Key::D) == WindowSystem::Press) mFPSCamera.move(step * mFPSCamera.getRight());
    if (WindowSystem::key(mWindow, WindowSystem::Key::Z) == WindowSystem::Press) mFPSCamera.move(step * glm::vec3(0, 1, 0));
    else if (WindowSystem::key(mWindow, WindowSystem::Key::X) == WindowSystem::Press) mFPSCamera.move(-step * glm::vec3(0, 1, 0));
}

void Application::render()
{
    if (mOptions.width <= 0 || mOptions.height <= 0) return;
    if (!mHDR->begin(mOptions.width,mOptions.height)) return;
    const Camera& camera = mFPSMode ? static_cast<const Camera&>(mFPSCamera) : mCinematicCamera;
    const auto view = camera.getViewMatrix();
    const auto projection = glm::perspective(glm::radians(camera.getFOV()),
        static_cast<float>(mOptions.width) / mOptions.height, 0.1f, MAX_DISTANCE);
    const auto position = camera.getPosition();
    SceneSetup::update(mScene, position);
    mScene.findObject("Earth")->visible = !mEarthBreakupSystem.active();
    mScene.findObject("EarthClouds")->visible = !mEarthBreakupSystem.active();
    mScene.findObject("Earth")->material.setFloat("destructionLevel", mEarthDamageSystem.destructionLevel());
    mRenderer.render(mScene, mLightManager, view, projection, position);
    const auto core = mEarthBreakupSystem.coreCollider();
    mMeteorSystem.render(mRenderer, mLightManager, view, projection, position,
        mEarthBreakupSystem.active() ? &core : nullptr);
    mEarthBreakupSystem.render(mRenderer,mScene.findObject("Earth")->material,mLightManager,view,projection,position);
    mParticleSystem.render(view, projection);
    mHDR->finish();
    if (mOptions.visible) WindowSystem::drawControls(mWindow, mSequencePaused, mMusicMuted);
}

void Application::showFPS(double currentTime)
{
    ++mFrameCount;
    double elapsed = currentTime - mFPSStart;
    if (elapsed <= 0.25) return;
    double fps = mFrameCount / elapsed;
    std::ostringstream title;
    title.precision(3);
    title << std::fixed << APP_TITLE << "    FPS: " << fps << "    Frame Time: " << 1000 / fps << " (ms)";
    title << "    Timeline: " << mTimeline.getTime() << " s  x" << mPlaybackRate
        << (mSequencePaused?" [pause]":"");
    title << "    Camera: " << (mFPSMode ? "FPS" : "Cinematic");
    if (mCameraDebug)
    {
        auto position = mFPSCamera.getPosition();
        title << "    Cam Pos: (" << position.x << ", " << position.y << ", " << position.z
              << ")    Yaw: " << mFPSCamera.getYaw() << " deg    Pitch: " << mFPSCamera.getPitch() << " deg";
    }
    WindowSystem::setTitle(mWindow, title.str().c_str());
    mFPSStart = currentTime;
    mFrameCount = 0;
}

void Application::keyCallback(WindowSystem::Window* window, int key, int, int action, int)
{
    if (auto* app = static_cast<Application*>(WindowSystem::userPointer(window))) app->onKey(key, action);
}

void Application::framebufferCallback(WindowSystem::Window* window, int width, int height)
{
    if (auto* app = static_cast<Application*>(WindowSystem::userPointer(window))) app->onFramebufferSize(width, height);
}

void Application::onKey(int key, int action)
{
    if (action != WindowSystem::Press) return;
    if (key == WindowSystem::Key::Escape) WindowSystem::close(mWindow);
    if (key == WindowSystem::Key::F1)
    {
        mWireframe = !mWireframe;
#ifndef PROJECT_MOBILE
        glPolygonMode(GL_FRONT_AND_BACK, mWireframe ? GL_LINE : GL_FILL);
#endif
    }
    if (key == WindowSystem::Key::G) mMoveSpeed *= 2;
    if (key == WindowSystem::Key::H) mMoveSpeed /= 2;
    if (key == WindowSystem::Key::F2) mCameraDebug = !mCameraDebug;
    if (key == WindowSystem::Key::F3)
    {
        mFPSMode = !mFPSMode;
        if (mFPSMode)
        {
            const auto& look = mCinematicCamera.getLook();
            mFPSCamera.setPosition(mCinematicCamera.getPosition());
            mFPSCamera.rotate(glm::degrees(std::atan2(look.x, look.z)) - mFPSCamera.getYaw(),
                glm::degrees(std::asin(glm::clamp(look.y, -1.0f, 1.0f))) - mFPSCamera.getPitch());
            mFPSCamera.setFOV(mCinematicCamera.getFOV());
        }
        int width, height;
        WindowSystem::size(mWindow, &width, &height);
        WindowSystem::setCursor(mWindow, width / 2.0, height / 2.0);
    }
    if (key == WindowSystem::Key::M) {
        mMusicMuted=!mMusicMuted; mMusic.setMuted(mMusicMuted);
    }
    if (key == WindowSystem::Key::Up || key == WindowSystem::Key::Equal || key == WindowSystem::Key::KeypadAdd) setPlaybackRate(mPlaybackRate*2);
    if (key == WindowSystem::Key::Down || key == WindowSystem::Key::Minus || key == WindowSystem::Key::KeypadSubtract) setPlaybackRate(mPlaybackRate/2);
    if (key == WindowSystem::Key::Zero || key == WindowSystem::Key::KeypadZero) setPlaybackRate(1);
    if (key == WindowSystem::Key::Left) seekSequence(mTimeline.getTime()-10);
    if (key == WindowSystem::Key::Right) seekSequence(mTimeline.getTime()+10);
    if (key == WindowSystem::Key::Space) toggleSequencePause();
    if (key == WindowSystem::Key::R) restartSequence();
}

void Application::restartSequence()
{
    if(!mInitialized) return;
    mSequencePaused=false;
    mMusic.setPaused(false);
    seekSequence(0);
}

void Application::setPlaybackRate(float rate)
{
    if(!std::isfinite(rate)) return;
    mPlaybackRate=std::clamp(rate,0.25f,8.0f);
    mMusic.setPlaybackRate(mPlaybackRate);
}

void Application::toggleSequencePause()
{
    if(!mInitialized) return;
    mClockResync=true;
    mSequencePaused=!mSequencePaused;
    mMusic.setPaused(mSequencePaused);
}

void Application::seekSequence(float seconds)
{
    if(!mInitialized || !std::isfinite(seconds)) return;
    seconds=std::clamp(seconds,0.0f,mTimeline.getDuration());
    mMusic.setPaused(true);
    resetSequenceState();
    // Rebuild impacts, heat, fragments and emission RNG as well as the camera.
    // Historical impact sounds are skipped; upload the maps only at the end.
    while(mTimeline.getTime()<seconds) simulate(std::min(1.0f/60,seconds-mTimeline.getTime()),false);
    mEarthDamageSystem.upload(mResources->earthDamageTexture,mResources->earthHeatTexture);
    mScene.findObject("Earth")->visible=!mEarthBreakupSystem.active();
    mScene.findObject("EarthClouds")->visible=!mEarthBreakupSystem.active();
    SceneSetup::update(mScene,mCinematicCamera.getPosition());
    mDebugTimer=0;
    if(mMusic.ready()) mMusic.seek(seconds);
    mMusic.setPaused(mSequencePaused);
    mClockResync=true;
}

void Application::resetSequenceState()
{
    if (!mInitialized) return;
    MainSequence::reset(mTimeline, mMeteorShower, mMeteorSystem);
    mImpactLightSystem.clear();
    mParticleSystem.clear();
    mImpactParticleEmitter.reset();
    mMeteorTrailEmitter.reset();
    mImpactLightSystem.publish(mLightManager);
    mEarthDamageSystem.clear();
    mEarthBreakupSystem.reset();
    mSolarSystem.reset(mScene);
    mScene.findObject("Earth")->visible = true;
    mScene.findObject("EarthClouds")->visible = true;
    mScene.findObject("Earth")->material.setFloat("destructionLevel", 0);
    mEarthDamageSystem.upload(mResources->earthDamageTexture, mResources->earthHeatTexture);
    auto sunlight=mLightManager.getDirectionalLight();
    sunlight.direction=glm::normalize(mScene.findObject("Earth")->transform.position-SolarSystem::SunCenter);
    mLightManager.setDirectionalLight(sunlight);
    SceneSetup::update(mScene,mCinematicCamera.getPosition());
    mTimeline.play();
    mDebugTimer = 0;
}

void Application::onFramebufferSize(int width, int height)
{
    mOptions.width = width;
    mOptions.height = height;
    glViewport(0, 0, width, height);
}

void Application::shutdown()
{
    WindowSystem::setAudioDevice(mWindow, 0);
    mMusic.release();
    if (mWindow) WindowSystem::bind(mWindow);
    mSequencePaused=false;
    mTimeline = Timeline{}; // Release borrowed scene/camera bindings first.
    mMeteorShower.reset();
    mMeteorSystem.clear();
    mImpactLightSystem.clear();
    mParticleSystem.clear();
    mImpactParticleEmitter.reset();
    mMeteorTrailEmitter.reset();
    mSolarSystem.reset(mScene);
    mEarthBreakupSystem.releaseGraphics();
    mHDR.reset();
    mMeteorSystem.releaseGraphics();
    mParticleSystem.releaseGraphics();
    mScene.objects.clear();
    mEarthDamageSystem.clear();
    mEarthBreakupSystem.reset();
    mResources.reset(); // GPU destructors require the current context.
    mLightManager = LightManager{};
    if (mWindow) WindowSystem::destroy(mWindow);
    mWindow = nullptr;
    mInitialized = false;
    mWireframe = false;
    mFPSMode = false;
    mFrameCount = 0;
    mDebugTimer = 0;
}
