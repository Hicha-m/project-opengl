#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <vector>
#include <iostream>
#include <cmath>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include "Application.h"
#include "scene/SceneSetup.h"
#include "cinematic/MainSequence.h"
#include "systems/MeteorShower.h"

static void capture(const char* path, int width, int height)
{
    std::vector<unsigned char> pixels(width * height * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    assert(glGetError() == GL_NO_ERROR);
    bool varied = false;
    for (std::size_t i = 3; i < pixels.size(); i += 3)
        if (pixels[i] != pixels[0] || pixels[i + 1] != pixels[1] || pixels[i + 2] != pixels[2]) varied = true;
    assert(varied); // Catch a blank frame rather than just a valid GL context.
    std::ofstream image(path, std::ios::binary);
    image << "P6\n" << width << " " << height << "\n255\n";
    for (int row = height - 1; row >= 0; --row)
        image.write(reinterpret_cast<const char*>(pixels.data() + row * width * 3), width * 3);
    assert(image.good());
}

static std::vector<unsigned char> pixels()
{
    std::vector<unsigned char> result(640 * 480 * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, 640, 480, GL_RGB, GL_UNSIGNED_BYTE, result.data());
    assert(glGetError() == GL_NO_ERROR);
    return result;
}

static void checkMeteors(MeteorSystem& system)
{
    Renderer renderer;
    LightManager lights;
    DirectionalLight light;
    light.direction = glm::normalize(glm::vec3(-1, -1, -1));
    lights.setDirectionalLight(light);
    const glm::vec3 eye(0, 0, 20);
    const auto view = glm::lookAt(eye, glm::vec3(0), glm::vec3(0, 1, 0));
    const auto projection = glm::ortho(-14.0f, 14.0f, -10.5f, 10.5f, 0.1f, 100.0f);
    auto draw = [&]()
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        system.render(renderer, lights, view, projection, eye);
    };
    assert(system.graphicsReady());
    assert(system.initGraphics());
    draw();
    const auto empty = pixels();
    GLint sharedProgram = 0;
    for (int count : {1, 10, 100, 500})
    {
        const int columns = int(std::ceil(std::sqrt(float(count))));
        const int rows = (count + columns - 1) / columns;
        const float spacing = count > 100 ? 0.8f : 1.0f;
        for (int i = 0; i < count; ++i)
        {
            Transform transform;
            transform.position = {float(i % columns) - (columns - 1) * 0.5f,
                float(i / columns) - (rows - 1) * 0.5f, 0};
            transform.position *= spacing;
            transform.scale = glm::vec3(0.3f);
            assert(system.spawn(transform, {i % 2 ? -0.2f : 0.2f, 0.1f, 0}, 2));
        }
        assert(system.size() == std::size_t(count));
        draw();
        GLint program = 0;
        glGetIntegerv(GL_CURRENT_PROGRAM, &program);
        if (!sharedProgram) sharedProgram = program;
        assert(program != 0 && program == sharedProgram);
        const auto start = pixels();
        assert(start != empty);
        auto path = "/tmp/meteors-" + std::to_string(count) + ".ppm";
        capture(path.c_str(), 640, 480);
        system.update(1);
        draw();
        assert(pixels() != start); // Actual rendered movement, not just CPU state.
        if (count == 100) capture("/tmp/meteors-100-moved.ppm", 640, 480);
        system.update(1);
        assert(system.size() == 0);
        draw();
        assert(pixels() == empty);
        system.clear();
        assert(system.graphicsReady() && glIsProgram(sharedProgram));
    }
    glUseProgram(0); // A bound program otherwise remains alive until unbound.
    system.releaseGraphics();
    system.releaseGraphics();
    assert(!system.graphicsReady() && !glIsProgram(sharedProgram));
    draw(); // Rendering without resources safely does nothing.
    assert(pixels() == empty);
    assert(system.initGraphics());
}

static void checkShower(MeteorSystem& system)
{
    system.clear();
    MeteorShower shower(system);
    MeteorShowerConfig config;
    config.spawnRate = 100;
    config.origin = {0, 7, 0};
    config.spawnHalfExtents = {10, 2, 2};
    config.direction = {0.3f, -1, 0};
    config.spreadRadians = 0.2f;
    config.minSpeed = 0.8f;
    config.maxSpeed = 1.5f;
    config.minScale = 0.1f;
    config.maxScale = 0.25f;
    config.minLifetime = 8;
    config.maxLifetime = 10;
    config.seed = 42;
    assert(shower.configure(config));
    Renderer renderer;
    LightManager lights;
    DirectionalLight light;
    light.direction = glm::normalize(glm::vec3(-1, -1, -1));
    lights.setDirectionalLight(light);
    const glm::vec3 eye(0, 0, 20);
    const auto view = glm::lookAt(eye, glm::vec3(0), glm::vec3(0, 1, 0));
    const auto projection = glm::ortho(-14.0f, 14.0f, -10.5f, 10.5f, 0.1f, 100.0f);
    auto draw = [&]()
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        system.render(renderer, lights, view, projection, eye);
    };
    draw();
    const auto empty = pixels();
    shower.start();
    for (int frame = 1; frame <= 300; ++frame)
    {
        // Emit at the frame boundary, after advancing already existing instances.
        system.update(1.0f / 60);
        shower.update(1.0f / 60);
        if (frame == 60 || frame == 300)
        {
            assert(system.size() == std::size_t(frame == 60 ? 100 : 500));
            draw();
            assert(pixels() != empty);
            capture(frame == 60 ? "/tmp/shower-100.ppm" : "/tmp/shower-500.ppm", 640, 480);
        }
    }
    const auto populated = pixels();
    shower.stop();
    shower.update(0.5f);
    system.update(0.5f);
    assert(system.size() == 500);
    draw();
    assert(pixels() != populated);
    capture("/tmp/shower-stopped-moving.ppm", 640, 480);
    system.update(20);
    draw();
    assert(system.size() == 0 && pixels() == empty);
}

int main()
{
    ApplicationOptions options;
    options.fullscreen = false;
    options.visible = false;
    options.width = 640;
    options.height = 480;
    Application app(options);
    app.restartSequence(); // Safe before initialization.
    assert(app.init());
    assert(app.init()); // Does not duplicate the scene or create another window.
    app.run(3);
    assert(glGetError() == GL_NO_ERROR);
    checkMeteors(app.meteors());
    checkShower(app.meteors());
    Transform meteor;
    meteor.position = {30, 50, 15};
    meteor.scale = glm::vec3(0.5f);
    assert(app.meteors().spawn(meteor, {1, 0, 0}, 100));
    app.run(3); // Also exercise Application's simulation and render integration.
    assert(app.meteors().size() == 1);
    assert(app.meteors().meteors()[0].transform.position.x > 30);
    app.restartSequence();
    assert(app.meteors().size() == 0);
    app.run(3);
    assert(app.meteors().size() == 0);
    {
        // Exercise the extracted content independently, with the same real context.
        SceneResources resources;
        Scene scene;
        LightManager lights;
        Renderer renderer;
        CinematicCamera camera;
        Timeline timeline;
        assert(SceneSetup::build(scene, lights, resources));
        assert(scene.getObjectCount() == 4);
        assert(scene.findObject("Earth")->mesh == &resources.earthSphere.getMesh());
        assert(scene.findObject("EarthClouds")->material.blending);
        assert(!scene.findObject("Stars")->material.depthWrite);
        assert(lights.getDirectionalLight().intensity == 1);
        MeteorSystem meteors;
        MeteorShower shower(meteors);
        assert(meteors.initGraphics());
        assert(MainSequence::build(timeline, camera, scene, shower));
        const int frames[] = {0, 60, 80, 88, 108, 120};
        const char* images[] = {"/tmp/space-start.ppm", "/tmp/space-middle.ppm",
            "/tmp/space-shower-stop.ppm", "/tmp/space-after-shower.ppm",
            "/tmp/space-meteors-expired.ppm", "/tmp/space-end.ppm"};
        std::vector<std::vector<unsigned char>> firstPass;
        for (int pass = 0; pass < 2; ++pass)
        {
            MainSequence::reset(timeline, shower, meteors);
            timeline.play();
            int imageIndex = 0;
            for (int frame = 0; frame <= 120; ++frame)
            {
                if (frame > 0)
                {
                    timeline.update(0.25f);
                    meteors.update(0.25f);
                    shower.update(0.25f);
                }
                if (frame < 40) assert(meteors.size() == 0 && !shower.isRunning());
                if (frame == 60) assert(meteors.size() > 100 && shower.isRunning());
                if (frame == 80 || frame == 88)
                    assert(meteors.size() > 0 && !shower.isRunning());
                if (frame >= 108) assert(meteors.size() == 0 && !shower.isRunning());
                if (imageIndex >= 6 || frame != frames[imageIndex]) continue;
                SceneSetup::update(scene, camera.getPosition());
                assert(scene.findObject("Stars")->transform.position == camera.getPosition());
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                const auto view = camera.getViewMatrix();
                const auto projection = glm::perspective(glm::radians(camera.getFOV()),
                    640.0f / 480, 0.1f, 100000000.0f);
                renderer.render(scene, lights, view, projection, camera.getPosition());
                const auto withoutMeteors = pixels();
                meteors.render(renderer, lights, view, projection, camera.getPosition());
                const auto rendered = pixels();
                if (meteors.size() > 0) assert(rendered != withoutMeteors);
                else assert(rendered == withoutMeteors);
                if (pass == 0)
                {
                    capture(images[imageIndex], 640, 480);
                    firstPass.push_back(rendered);
                }
                else assert(rendered == firstPass[imageIndex]); // Actual image replay.
                ++imageIndex;
            }
            assert(imageIndex == 6 && !timeline.isPlaying() && timeline.getTime() == 30);
        }
    } // All these GPU resources are released while the context is alive.
    assert(glGetError() == GL_NO_ERROR);
    app.shutdown();
    app.shutdown();
    assert(app.meteors().size() == 0 && !app.meteors().graphicsReady());
    assert(glfwGetCurrentContext() == nullptr);
    assert(app.init());
    assert(app.meteors().size() == 0 && app.meteors().graphicsReady());
    app.run(3);
    app.shutdown();

    // Missing assets must fail cleanly, then allow recovery on the same instance.
    auto original = std::filesystem::current_path();
    auto empty = std::filesystem::temp_directory_path() /
        ("space-empty-assets-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(empty);
    std::filesystem::current_path(empty);
    assert(!app.init());
    assert(glfwGetCurrentContext() == nullptr);
    // Valid shaders but missing textures exercise partial GPU resource cleanup too.
    std::filesystem::create_directory_symlink(original / "shaders", empty / "shaders");
    assert(!app.init());
    assert(glfwGetCurrentContext() == nullptr);
    // Scene assets present, meteor shader absent: clean up the whole partial init.
    std::filesystem::remove(empty / "shaders"); // Remove only the temporary symlink.
    std::filesystem::create_directory(empty / "shaders");
    for (const auto& file : std::filesystem::directory_iterator(original / "shaders"))
        if (file.path().filename() != "meteor.frag")
            std::filesystem::create_symlink(file.path(), empty / "shaders" / file.path().filename());
    std::filesystem::create_directory_symlink(original / "textures", empty / "textures");
    assert(!app.init());
    assert(glfwGetCurrentContext() == nullptr);
    assert(!app.meteors().graphicsReady() && app.meteors().size() == 0);
    std::filesystem::current_path(original);
    std::filesystem::remove_all(empty);
    assert(app.init());
    app.run(3);
    std::cout << "Application lifecycle, cinematic shower and image replay checks passed\n";
} // Application destructor also releases resources before GLFW.
