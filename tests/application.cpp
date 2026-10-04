#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <vector>
#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include "src/Application.h"
#include "src/scene/SceneSetup.h"
#include "src/cinematic/MainSequence.h"

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

int main()
{
    ApplicationOptions options;
    options.fullscreen = false;
    options.visible = false;
    options.width = 640;
    options.height = 480;
    Application app(options);
    assert(app.init());
    assert(app.init()); // Does not duplicate the scene or create another window.
    app.run(3);
    assert(glGetError() == GL_NO_ERROR);
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
        assert(MainSequence::build(timeline, camera, scene));
        const float times[] = {0, 15, 30};
        const char* images[] = {"/tmp/space-start.ppm", "/tmp/space-middle.ppm", "/tmp/space-end.ppm"};
        float previous = 0;
        for (int i = 0; i < 3; ++i)
        {
            timeline.update(times[i] - previous);
            previous = times[i];
            SceneSetup::update(scene, camera.getPosition());
            assert(scene.findObject("Stars")->transform.position == camera.getPosition());
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            renderer.render(scene, lights, camera.getViewMatrix(),
                glm::perspective(glm::radians(camera.getFOV()), 640.0f / 480, 0.1f, 100000000.0f),
                camera.getPosition());
            capture(images[i], 640, 480);
        }
        assert(!timeline.isPlaying());
    } // All these GPU resources are released while the context is alive.
    assert(glGetError() == GL_NO_ERROR);
    app.shutdown();
    app.shutdown();
    assert(glfwGetCurrentContext() == nullptr);
    assert(app.init());
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
    std::filesystem::current_path(original);
    std::filesystem::remove_all(empty);
    assert(app.init());
    app.run(3);
    std::cout << "Application lifecycle and 30-second rendering checks passed\n";
} // Application destructor also releases resources before GLFW.
