#include "Application.h"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <sstream>
#include <cmath>
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
    if (!SceneSetup::build(mScene, mLightManager, *mResources)
        || !mMeteorSystem.initGraphics()
        || !mParticleSystem.initGraphics()
        || !MainSequence::build(mTimeline, mCinematicCamera, mScene, mMeteorShower))
    {
        std::cerr << "Scene initialization failed\n";
        shutdown();
        return false;
    }
    mFPSStart = glfwGetTime();
    mInitialized = true;
    return true;
}

bool Application::initOpenGL()
{
    if (mOptions.width <= 0 || mOptions.height <= 0) return false;
    glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
    if (!glfwInit())
    {
        std::cerr << "GLFW initialization failed\n";
        return false;
    }
    mGLFWInitialized = true;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_VISIBLE, mOptions.visible ? GLFW_TRUE : GLFW_FALSE);

    GLFWmonitor* monitor = mOptions.fullscreen ? glfwGetPrimaryMonitor() : nullptr;
    int width = mOptions.width, height = mOptions.height;
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
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
    {
        std::cerr << "Failed to initialize GLEW\n";
        return false;
    }
    // GLEW may leave GL_INVALID_ENUM when probing a core context.
    while (glGetError() != GL_NO_ERROR) {}
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
    if (!mInitialized) return;
    double lastTime = glfwGetTime();
    std::size_t frames = 0;
    while (!glfwWindowShouldClose(mWindow))
    {
        double currentTime = glfwGetTime();
        glfwPollEvents();
        update(static_cast<float>(currentTime - lastTime));
        render();
        showFPS(currentTime);
        glfwSwapBuffers(mWindow);
        lastTime = currentTime;
        if (frameLimit && ++frames >= frameLimit) break;
    }
}

void Application::update(float deltaTime)
{
    updateInput(deltaTime);
    // Events affect this frame. Existing instances move first; births happen at
    // the frame boundary and begin moving on the next frame.
    mTimeline.update(deltaTime);
    mMeteorTrailEmitter.observe(mMeteorSystem.meteors());
    mMeteorSystem.update(deltaTime, SceneSetup::earthCollider(mScene, *mResources));
    // Advance existing particles before births, so every burst is visible at age zero.
    mParticleSystem.update(deltaTime);
    mMeteorTrailEmitter.update(mMeteorSystem.meteors(), deltaTime);
    const auto* earth = mScene.findObject("Earth");
    mEarthDamageSystem.consume(mMeteorSystem.impacts(), earth->transform, mResources->earthSphere.getRadius());
    mEarthDamageSystem.upload(mResources->earthDamageTexture);
    mImpactParticleEmitter.consume(mMeteorSystem.impacts());
    mImpactLightSystem.consume(mMeteorSystem.impacts());
    mImpactLightSystem.update(deltaTime);
    mImpactLightSystem.publish(mLightManager);
    mMeteorShower.update(deltaTime);
    mDebugTimer += deltaTime;
    if (mDebugTimer >= 1.0)
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
    glfwGetWindowSize(mWindow, &width, &height);
    double mouseX, mouseY;
    glfwGetCursorPos(mWindow, &mouseX, &mouseY);
    glfwSetCursorPos(mWindow, width / 2.0, height / 2.0);
    if (!mFPSMode) return;
    mFPSCamera.rotate(static_cast<float>(width / 2.0 - mouseX) * MOUSE_SENSITIVITY,
                      static_cast<float>(height / 2.0 - mouseY) * MOUSE_SENSITIVITY);
    const float step = mMoveSpeed * deltaTime;
    if (glfwGetKey(mWindow, GLFW_KEY_W) == GLFW_PRESS) mFPSCamera.move(step * mFPSCamera.getLook());
    else if (glfwGetKey(mWindow, GLFW_KEY_S) == GLFW_PRESS) mFPSCamera.move(-step * mFPSCamera.getLook());
    if (glfwGetKey(mWindow, GLFW_KEY_A) == GLFW_PRESS) mFPSCamera.move(-step * mFPSCamera.getRight());
    else if (glfwGetKey(mWindow, GLFW_KEY_D) == GLFW_PRESS) mFPSCamera.move(step * mFPSCamera.getRight());
    if (glfwGetKey(mWindow, GLFW_KEY_Z) == GLFW_PRESS) mFPSCamera.move(step * glm::vec3(0, 1, 0));
    else if (glfwGetKey(mWindow, GLFW_KEY_X) == GLFW_PRESS) mFPSCamera.move(-step * glm::vec3(0, 1, 0));
}

void Application::render()
{
    if (mOptions.width <= 0 || mOptions.height <= 0) return;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    const Camera& camera = mFPSMode ? static_cast<const Camera&>(mFPSCamera) : mCinematicCamera;
    const auto view = camera.getViewMatrix();
    const auto projection = glm::perspective(glm::radians(camera.getFOV()),
        static_cast<float>(mOptions.width) / mOptions.height, 0.1f, MAX_DISTANCE);
    const auto position = camera.getPosition();
    SceneSetup::update(mScene, position);
    mRenderer.render(mScene, mLightManager, view, projection, position);
    mMeteorSystem.render(mRenderer, mLightManager, view, projection, position);
    mParticleSystem.render(view, projection);
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

void Application::keyCallback(GLFWwindow* window, int key, int, int action, int)
{
    if (auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window))) app->onKey(key, action);
}

void Application::framebufferCallback(GLFWwindow* window, int width, int height)
{
    if (auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window))) app->onFramebufferSize(width, height);
}

void Application::onKey(int key, int action)
{
    if (action != GLFW_PRESS) return;
    if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(mWindow, GLFW_TRUE);
    if (key == GLFW_KEY_F1)
    {
        mWireframe = !mWireframe;
        glPolygonMode(GL_FRONT_AND_BACK, mWireframe ? GL_LINE : GL_FILL);
    }
    if (key == GLFW_KEY_G) mMoveSpeed *= 2;
    if (key == GLFW_KEY_H) mMoveSpeed /= 2;
    if (key == GLFW_KEY_F2) mCameraDebug = !mCameraDebug;
    if (key == GLFW_KEY_F3)
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
        glfwGetWindowSize(mWindow, &width, &height);
        glfwSetCursorPos(mWindow, width / 2.0, height / 2.0);
    }
    if (key == GLFW_KEY_R) restartSequence();
}

void Application::restartSequence()
{
    if (!mInitialized) return;
    MainSequence::reset(mTimeline, mMeteorShower, mMeteorSystem);
    mImpactLightSystem.clear();
    mParticleSystem.clear();
    mImpactParticleEmitter.reset();
    mMeteorTrailEmitter.reset();
    mImpactLightSystem.publish(mLightManager);
    mEarthDamageSystem.clear();
    mEarthDamageSystem.upload(mResources->earthDamageTexture);
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
    if (mWindow) glfwMakeContextCurrent(mWindow);
    mTimeline = Timeline{}; // Release borrowed scene/camera bindings first.
    mMeteorShower.reset();
    mMeteorSystem.clear();
    mImpactLightSystem.clear();
    mParticleSystem.clear();
    mImpactParticleEmitter.reset();
    mMeteorTrailEmitter.reset();
    mMeteorSystem.releaseGraphics();
    mParticleSystem.releaseGraphics();
    mScene.objects.clear();
    mEarthDamageSystem.clear();
    mResources.reset(); // GPU destructors require the current context.
    mLightManager = LightManager{};
    if (mWindow) glfwDestroyWindow(mWindow);
    mWindow = nullptr;
    if (mGLFWInitialized) glfwTerminate();
    mGLFWInitialized = false;
    mInitialized = false;
    mWireframe = false;
    mFPSMode = false;
    mFrameCount = 0;
    mDebugTimer = 0;
}
