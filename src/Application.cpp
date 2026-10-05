#include "Application.h"
#include "cinematic/MainSequence.h"
#include "scene/SceneSetup.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace
{
    constexpr const char* APP_TITLE = "Project - Space";
    constexpr float MOUSE_SENSITIVITY = 0.1f;
    constexpr float MAX_DISTANCE = 100000000.0f;
} // namespace

Application::Application(ApplicationOptions options)
    : mOptions(options), mFPSCamera(glm::vec3(0, 3.5f, 10), glm::radians(110.0f), glm::radians(55.0f))
{
}

Application::~Application()
{
    shutdown();
}

bool Application::init()
{
    if (mInitialized)
    {
        return true;
    }
    if (!initOpenGL())
    {
        shutdown();
        return false;
    }
    mResources = std::make_unique<SceneResources>();
    mHDR = std::make_unique<HDRPipeline>();
    if (!SceneSetup::build(mScene, mLightManager, *mResources) ||
        !mEarthBreakupSystem.initGraphics(mResources->earthSphere.getMesh()) ||
        !mHDR->init(mOptions.width, mOptions.height) || !mMeteorSystem.initGraphics() ||
        !mParticleSystem.initGraphics() ||
        !MainSequence::build(mTimeline, mCinematicCamera, mScene, mMeteorShower, &mSolarSystem,
                             &mEarthBreakupSystem))
    {
        std::cerr << "Scene initialization failed\n";
        shutdown();
        return false;
    }
    if (mOptions.music && mMusic.load("build/music/cinematic.wav"))
    {
        mTimeline.setDuration(mMusic.duration());
        if (!mMusic.loadImpact("build/music/impact.wav"))
        {
            std::cerr << "Impact sound unavailable\n";
        }
    }
    else if (mOptions.music)
    {
        std::cerr << "Music unavailable; continuing without audio\n";
    }
    mFPSStart = glfwGetTime();
    mInitialized = true;
    return true;
}

bool Application::initOpenGL()
{
    if (mOptions.width <= 0 || mOptions.height <= 0)
    {
        return false;
    }
    // Let GLFW select Win32, Cocoa, X11 or Wayland according to the host.
    glfwSetErrorCallback([](int code, const char* description)
                         { std::cerr << "GLFW error " << code << ": " << description << '\n'; });
#if GLFW_VERSION_MAJOR > 3 || (GLFW_VERSION_MAJOR == 3 && GLFW_VERSION_MINOR >= 4)
    glfwInitHint(GLFW_PLATFORM, mOptions.softwareContext ? GLFW_PLATFORM_NULL : GLFW_ANY_PLATFORM);
#endif
    bool initialized = glfwInit() == GLFW_TRUE;
#if defined(__linux__) && (GLFW_VERSION_MAJOR > 3 || (GLFW_VERSION_MAJOR == 3 && GLFW_VERSION_MINOR >= 4))
    // A stale Wayland socket can coexist with a working X11/XWayland display.
    if (!initialized && !mOptions.softwareContext && glfwPlatformSupported(GLFW_PLATFORM_X11))
    {
        glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
        initialized = glfwInit() == GLFW_TRUE;
    }
#endif
    if (!initialized)
    {
        std::cerr << "GLFW initialization failed\n";
        return false;
    }
    mGLFWInitialized = true;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, mOptions.softwareContext ? GLFW_FALSE : GLFW_TRUE);
    if (mOptions.softwareContext)
    {
        glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_OSMESA_CONTEXT_API);
    }
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_VISIBLE, mOptions.visible ? GLFW_TRUE : GLFW_FALSE);

    GLFWmonitor* monitor = mOptions.fullscreen ? glfwGetPrimaryMonitor() : nullptr;
    int width = mOptions.width, height = mOptions.height;
    mWindowedWidth = width;
    mWindowedHeight = height;
    if (monitor)
    {
        if (const auto* mode = glfwGetVideoMode(monitor))
        {
            width = mode->width;
            height = mode->height;
        }
    }
    mWindow = glfwCreateWindow(width, height, APP_TITLE, monitor, nullptr);
    if (!mWindow)
    {
        std::cerr << "Failed to create GLFW window\n";
        return false;
    }
    glfwMakeContextCurrent(mWindow);
    if (!gladLoadGL(reinterpret_cast<GLADloadfunc>(glfwGetProcAddress)) || !GLAD_GL_VERSION_3_3)
    {
        std::cerr << "Failed to load OpenGL 3.3 functions\n";
        return false;
    }
    glfwSetWindowUserPointer(mWindow, this);
    glfwSetKeyCallback(mWindow, keyCallback);
    glfwSetFramebufferSizeCallback(mWindow, framebufferCallback);
    glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPos(mWindow, width / 2.0, height / 2.0);
    glfwGetFramebufferSize(mWindow, &width, &height);
    onFramebufferSize(width, height);
    glClearColor(0.06f, 0.06f, 0.07f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    return true;
}

void Application::run(std::size_t frameLimit)
{
    if (!mInitialized)
    {
        return;
    }
    if (mMusic.ready() && !mMusic.running())
    {
        mMusic.restart();
    }
    double lastTime = glfwGetTime();
    std::size_t frames = 0;
    while (!glfwWindowShouldClose(mWindow))
    {
        glfwPollEvents();
        double currentTime = glfwGetTime();
        if (mClockResync)
        {
            lastTime = currentTime;
            mClockResync = false;
        }
        update(static_cast<float>(currentTime - lastTime));
        render();
        showFPS(currentTime);
        glfwSwapBuffers(mWindow);
        lastTime = currentTime;
        if (frameLimit && ++frames >= frameLimit)
        {
            break;
        }
    }
}

void Application::renderVideo(int width, int height, int fps, double duration,
                              const std::function<void(const unsigned char*, std::size_t)>& writeFrame,
                              const std::function<void(float, float)>& writeImpact)
{
    if (!mInitialized || width <= 0 || height <= 0 || fps <= 0 || !std::isfinite(duration) || duration <= 0)
    {
        throw std::runtime_error("Invalid video export settings");
    }
    glfwSetKeyCallback(mWindow, nullptr);
    glfwSetFramebufferSizeCallback(mWindow, nullptr);
    mMusic.setPaused(true);
    mTimeline.setDuration(static_cast<float>(duration));
    resetSequenceState();
    mFPSMode = mWireframe = false;
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    // Capture an offscreen target, independent of desktop size and HiDPI scaling.
    struct CaptureTarget
    {
        GLuint framebuffer = 0, texture = 0;
        ~CaptureTarget()
        {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glDeleteFramebuffers(1, &framebuffer);
            glDeleteTextures(1, &texture);
        }
    } capture;
    GLint maxTextureSize;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
    if (width > maxTextureSize || height > maxTextureSize)
    {
        throw std::runtime_error("Export resolution exceeds GPU texture limits");
    }
    glGenTextures(1, &capture.texture);
    glBindTexture(GL_TEXTURE_2D, capture.texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenFramebuffers(1, &capture.framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, capture.framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, capture.texture, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        throw std::runtime_error("Cannot allocate video capture framebuffer");
    }
    onFramebufferSize(width, height);
    glfwSwapInterval(0);
    mExportImpact = writeImpact;
    mExportLastImpact = -1;
    std::vector<unsigned char> pixels(std::size_t(width) * height * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    const std::size_t count = static_cast<std::size_t>(std::ceil(duration * fps));
    double simulated = 0;
    for (std::size_t frame = 0; frame < count; ++frame)
    {
        glfwPollEvents();
        if (glfwWindowShouldClose(mWindow))
        {
            throw std::runtime_error("Video export cancelled");
        }
        const double target = double(frame) / fps;
        while (simulated < target)
        {
            const float step = static_cast<float>(std::min(1.0 / 60, target - simulated));
            simulate(step, false);
            simulated += step;
        }
        mEarthDamageSystem.upload(mResources->earthDamageTexture, mResources->earthHeatTexture);
        render();
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
        if (glGetError() != GL_NO_ERROR)
        {
            throw std::runtime_error("Video frame capture failed");
        }
        writeFrame(pixels.data(), pixels.size());
        if (frame % fps == 0)
        {
            std::cerr << "Export: " << frame / fps << " / " << duration << " s\n";
        }
    }
    mExportImpact = {};
}

void Application::update(float deltaTime)
{
    updateInput(deltaTime);
    if (mSequencePaused || !std::isfinite(deltaTime) || deltaTime <= 0)
    {
        return;
    }
    const bool finished = mTimeline.getTime() >= mTimeline.getDuration();
    deltaTime *= mPlaybackRate;
    if (mMusic.running() && !finished)
    {
        deltaTime = std::max(0.0f, mMusic.position() - mTimeline.getTime());
    }
    if (finished)
    {
        MainSequence::continueEscape(mScene, mCinematicCamera, deltaTime);
    }
    // Bound physics steps even at x8 or after a slow rendering frame.
    while (deltaTime > 0)
    {
        const float step = std::min(deltaTime, 1.0f / 60);
        simulate(step, true);
        deltaTime -= step;
    }
    mEarthDamageSystem.upload(mResources->earthDamageTexture, mResources->earthHeatTexture);
}

void Application::simulate(float deltaTime, bool audible)
{
    mTimeline.update(deltaTime);
    updateSunlight();
    mMeteorTrailEmitter.observe(mMeteorSystem.meteors());
    const bool hittingCore = mEarthBreakupSystem.active();
    if (hittingCore)
    {
        mMeteorSystem.update(deltaTime, mEarthBreakupSystem.coreCollider());
    }
    else
    {
        mMeteorSystem.update(deltaTime, SceneSetup::earthCollider(mScene, *mResources));
    }
    // Advance existing particles before births, so every burst is visible at age zero.
    mParticleSystem.update(deltaTime);
    mMeteorTrailEmitter.update(mMeteorSystem.meteors(), deltaTime);
    const auto* earth = mScene.findObject("Earth");
    mEarthDamageSystem.update(deltaTime);
    if (!hittingCore)
    {
        mEarthDamageSystem.consume(mMeteorSystem.impacts(), earth->transform,
                                   mResources->earthSphere.getRadius());
    }
    mEarthBreakupSystem.update(deltaTime, mEarthDamageSystem.destructionLevel(), earth->transform);
    // Core contact only absorbs meteors; Earth effects belong to the intact planet.
    if (!hittingCore)
    {
        dispatchImpactEffects(audible);
    }
    mImpactLightSystem.update(deltaTime);
    mImpactLightSystem.publish(mLightManager);
    mEarthBreakupSystem.publish(mLightManager);
    // No invisible Earth collider or new bombardment after the physical rupture.
    if (mEarthBreakupSystem.active())
    {
        mMeteorShower.stop();
    }
    mMeteorShower.update(deltaTime);
}

void Application::updateSunlight()
{
    auto sunlight = mLightManager.getDirectionalLight();
    sunlight.direction =
        glm::normalize(mScene.findObject("Earth")->transform.position - SolarSystem::SunCenter);
    mLightManager.setDirectionalLight(sunlight);
}

void Application::dispatchImpactEffects(bool audible)
{
    mImpactParticleEmitter.consume(mMeteorSystem.impacts());
    mImpactLightSystem.consume(mMeteorSystem.impacts());
    if (mExportImpact && mTimeline.getTime() - mExportLastImpact >= 0.12f)
    {
        const float gain = MusicPlayer::impactGain(mMeteorSystem.impacts(), mCinematicCamera.getPosition());
        if (gain >= 0.01f)
        {
            mExportImpact(mTimeline.getTime(), gain);
            mExportLastImpact = mTimeline.getTime();
        }
    }
    if (audible)
    {
        mMusic.playImpacts(mMeteorSystem.impacts(),
                           mFPSMode ? mFPSCamera.getPosition() : mCinematicCamera.getPosition(),
                           mTimeline.getTime());
    }
}

void Application::updateInput(float deltaTime)
{
    int width, height;
    glfwGetWindowSize(mWindow, &width, &height);
    double mouseX, mouseY;
    glfwGetCursorPos(mWindow, &mouseX, &mouseY);
    glfwSetCursorPos(mWindow, width / 2.0, height / 2.0);
    if (!mFPSMode)
    {
        return;
    }
    mFPSCamera.rotate(static_cast<float>(width / 2.0 - mouseX) * MOUSE_SENSITIVITY,
                      static_cast<float>(height / 2.0 - mouseY) * MOUSE_SENSITIVITY);
    const float step = mMoveSpeed * deltaTime;
    if (glfwGetKey(mWindow, GLFW_KEY_W) == GLFW_PRESS)
    {
        mFPSCamera.move(step * mFPSCamera.getLook());
    }
    else if (glfwGetKey(mWindow, GLFW_KEY_S) == GLFW_PRESS)
    {
        mFPSCamera.move(-step * mFPSCamera.getLook());
    }
    if (glfwGetKey(mWindow, GLFW_KEY_A) == GLFW_PRESS)
    {
        mFPSCamera.move(-step * mFPSCamera.getRight());
    }
    else if (glfwGetKey(mWindow, GLFW_KEY_D) == GLFW_PRESS)
    {
        mFPSCamera.move(step * mFPSCamera.getRight());
    }
    if (glfwGetKey(mWindow, GLFW_KEY_Z) == GLFW_PRESS)
    {
        mFPSCamera.move(step * glm::vec3(0, 1, 0));
    }
    else if (glfwGetKey(mWindow, GLFW_KEY_X) == GLFW_PRESS)
    {
        mFPSCamera.move(-step * glm::vec3(0, 1, 0));
    }
}

void Application::render()
{
    if (mOptions.width <= 0 || mOptions.height <= 0)
    {
        return;
    }
    if (!mHDR->begin(mOptions.width, mOptions.height))
    {
        return;
    }
    const Camera& camera = mFPSMode ? static_cast<const Camera&>(mFPSCamera) : mCinematicCamera;
    const auto view = camera.getViewMatrix();
    const auto projection =
        glm::perspective(glm::radians(camera.getFOV()), static_cast<float>(mOptions.width) / mOptions.height,
                         0.1f, MAX_DISTANCE);
    const auto position = camera.getPosition();
    SceneSetup::update(mScene, position);
    mScene.findObject("Earth")->visible = !mEarthBreakupSystem.active();
    mScene.findObject("EarthClouds")->visible = !mEarthBreakupSystem.active();
    mScene.findObject("Earth")->material.setFloat("destructionLevel", mEarthDamageSystem.destructionLevel());
    mRenderer.render(mScene, mLightManager, view, projection, position);
    const auto core = mEarthBreakupSystem.coreCollider();
    mMeteorSystem.render(mRenderer, mLightManager, view, projection, position,
                         mEarthBreakupSystem.active() ? &core : nullptr);
    mEarthBreakupSystem.render(mRenderer, mScene.findObject("Earth")->material, mLightManager, view,
                               projection, position);
    mParticleSystem.render(view, projection);
    mHDR->finish();
}

void Application::showFPS(double currentTime)
{
    ++mFrameCount;
    double elapsed = currentTime - mFPSStart;
    if (elapsed <= 0.25)
    {
        return;
    }
    double fps = mFrameCount / elapsed;
    std::ostringstream title;
    title.precision(3);
    title << std::fixed << APP_TITLE << "    FPS: " << fps << "    Frame Time: " << 1000 / fps << " (ms)";
    title << "    Timeline: " << mTimeline.getTime() << " s  x" << mPlaybackRate
          << (mSequencePaused ? " [pause]" : "");
    title << "    Camera: " << (mFPSMode ? "FPS" : "Cinematic");
    if (mCameraDebug)
    {
        auto position = mFPSCamera.getPosition();
        title << "    Cam Pos: (" << position.x << ", " << position.y << ", " << position.z
              << ")    Yaw: " << mFPSCamera.getYaw() << " deg    Pitch: " << mFPSCamera.getPitch() << " deg";
    }
    glfwSetWindowTitle(mWindow, title.str().c_str());
    mFPSStart = currentTime;
    mFrameCount = 0;
}

void Application::toggleFullscreen()
{
    if (glfwGetWindowMonitor(mWindow))
    {
        glfwSetWindowMonitor(mWindow, nullptr, mWindowedX, mWindowedY, mWindowedWidth, mWindowedHeight,
                             GLFW_DONT_CARE);
    }
    else
    {
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        if (!monitor)
        {
            return;
        }
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        if (!mode)
        {
            return;
        }
        glfwGetWindowPos(mWindow, &mWindowedX, &mWindowedY);
        glfwGetWindowSize(mWindow, &mWindowedWidth, &mWindowedHeight);
        glfwSetWindowMonitor(mWindow, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    }
    mOptions.fullscreen = glfwGetWindowMonitor(mWindow) != nullptr;
    int width, height;
    glfwGetFramebufferSize(mWindow, &width, &height);
    onFramebufferSize(width, height);
    glfwGetWindowSize(mWindow, &width, &height);
    glfwSetCursorPos(mWindow, width / 2.0, height / 2.0);
}

void Application::keyCallback(GLFWwindow* window, int key, int, int action, int)
{
    if (auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window)))
    {
        app->onKey(key, action);
    }
}

void Application::framebufferCallback(GLFWwindow* window, int width, int height)
{
    if (auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window)))
    {
        app->onFramebufferSize(width, height);
    }
}

void Application::onKey(int key, int action)
{
    if (action != GLFW_PRESS)
    {
        return;
    }
    if (key == GLFW_KEY_ESCAPE)
    {
        glfwSetWindowShouldClose(mWindow, GLFW_TRUE);
    }
    if (key == GLFW_KEY_F11)
    {
        toggleFullscreen();
    }
    if (key == GLFW_KEY_F1)
    {
        mWireframe = !mWireframe;
        glPolygonMode(GL_FRONT_AND_BACK, mWireframe ? GL_LINE : GL_FILL);
    }
    if (key == GLFW_KEY_G)
    {
        mMoveSpeed *= 2;
    }
    if (key == GLFW_KEY_H)
    {
        mMoveSpeed /= 2;
    }
    if (key == GLFW_KEY_F2)
    {
        mCameraDebug = !mCameraDebug;
    }
    if (key == GLFW_KEY_F3)
    {
        mFPSMode = !mFPSMode;
        if (mFPSMode)
        {
            const auto& look = mCinematicCamera.getLook();
            mFPSCamera.setPosition(mCinematicCamera.getPosition());
            mFPSCamera.rotate(glm::degrees(std::atan2(look.x, look.z)) - mFPSCamera.getYaw(),
                              glm::degrees(std::asin(glm::clamp(look.y, -1.0f, 1.0f))) -
                                  mFPSCamera.getPitch());
            mFPSCamera.setFOV(mCinematicCamera.getFOV());
        }
        int width, height;
        glfwGetWindowSize(mWindow, &width, &height);
        glfwSetCursorPos(mWindow, width / 2.0, height / 2.0);
    }
    if (key == GLFW_KEY_M)
    {
        mMusicMuted = !mMusicMuted;
        mMusic.setMuted(mMusicMuted);
    }
    if (key == GLFW_KEY_UP || key == GLFW_KEY_EQUAL || key == GLFW_KEY_KP_ADD)
    {
        setPlaybackRate(mPlaybackRate * 2);
    }
    if (key == GLFW_KEY_DOWN || key == GLFW_KEY_MINUS || key == GLFW_KEY_KP_SUBTRACT)
    {
        setPlaybackRate(mPlaybackRate / 2);
    }
    if (key == GLFW_KEY_0 || key == GLFW_KEY_KP_0)
    {
        setPlaybackRate(1);
    }
    if (key == GLFW_KEY_LEFT)
    {
        seekSequence(mTimeline.getTime() - 10);
    }
    if (key == GLFW_KEY_RIGHT)
    {
        seekSequence(mTimeline.getTime() + 10);
    }
    if (key == GLFW_KEY_SPACE)
    {
        toggleSequencePause();
    }
    if (key == GLFW_KEY_R)
    {
        restartSequence();
    }
}

void Application::restartSequence()
{
    if (!mInitialized)
    {
        return;
    }
    mSequencePaused = false;
    mMusic.setPaused(false);
    seekSequence(0);
}

void Application::setPlaybackRate(float rate)
{
    if (!std::isfinite(rate))
    {
        return;
    }
    mPlaybackRate = std::clamp(rate, 0.25f, 8.0f);
    mMusic.setPlaybackRate(mPlaybackRate);
}

void Application::toggleSequencePause()
{
    if (!mInitialized)
    {
        return;
    }
    mClockResync = true;
    mSequencePaused = !mSequencePaused;
    mMusic.setPaused(mSequencePaused);
}

void Application::seekSequence(float seconds)
{
    if (!mInitialized || !std::isfinite(seconds))
    {
        return;
    }
    seconds = std::clamp(seconds, 0.0f, mTimeline.getDuration());
    mMusic.setPaused(true);
    resetSequenceState();
    // Rebuild impacts, heat, fragments and emission RNG as well as the camera.
    // Historical impact sounds are skipped; upload the maps only at the end.
    while (mTimeline.getTime() < seconds)
    {
        simulate(std::min(1.0f / 60, seconds - mTimeline.getTime()), false);
    }
    mEarthDamageSystem.upload(mResources->earthDamageTexture, mResources->earthHeatTexture);
    mScene.findObject("Earth")->visible = !mEarthBreakupSystem.active();
    mScene.findObject("EarthClouds")->visible = !mEarthBreakupSystem.active();
    SceneSetup::update(mScene, mCinematicCamera.getPosition());
    if (mMusic.ready())
    {
        mMusic.seek(seconds);
    }
    mMusic.setPaused(mSequencePaused);
    mClockResync = true;
}

void Application::resetSequenceState()
{
    if (!mInitialized)
    {
        return;
    }
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
    updateSunlight();
    SceneSetup::update(mScene, mCinematicCamera.getPosition());
    mTimeline.play();
}

void Application::onFramebufferSize(int width, int height)
{
    mOptions.width = width;
    mOptions.height = height;
    glViewport(0, 0, width, height);
}

void Application::shutdown()
{
    mMusic.release();
    if (mWindow)
    {
        glfwMakeContextCurrent(mWindow);
    }
    mSequencePaused = false;
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
    if (mWindow)
    {
        glfwDestroyWindow(mWindow);
    }
    mWindow = nullptr;
    if (mGLFWInitialized)
    {
        glfwTerminate();
    }
    mGLFWInitialized = false;
    mInitialized = false;
    mWireframe = false;
    mFPSMode = false;
    mFrameCount = 0;
}
