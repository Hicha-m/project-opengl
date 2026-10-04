#include <cassert>
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <vector>
#include <iostream>
#include <cmath>
#include <stdexcept>
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

static void checkParticles(ParticleSystem& system)
{
    assert(system.graphicsReady() && system.initGraphics());
    system.clear();
    ParticleEmitter emitter(system);
    ParticleBurstConfig config;
    config.seed = 42;
    config.minLifetime = config.maxLifetime = 2;
    const auto view = glm::lookAt(glm::vec3(0,0,20), glm::vec3(0), glm::vec3(0,1,0));
    const auto projection = glm::ortho(-5.0f,5.0f,-3.75f,3.75f,0.1f,100.0f);
    auto draw = [&]() {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        system.render(view, projection);
    };
    draw(); const auto empty = pixels();
    for (std::size_t count : {100,1000,10000}) {
        config.count = count; assert(emitter.configure(config));
        assert(emitter.burst({0,0,0}, {0,1,0}));
        draw(); const auto birth = pixels(); assert(birth != empty);
        system.update(0.4f);
        // A primitives query verifies the complete population reaches one batch.
        GLuint query; glGenQueries(1, &query);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glBeginQuery(GL_PRIMITIVES_GENERATED, query);
        system.render(view, projection);
        glEndQuery(GL_PRIMITIVES_GENERATED);
        GLuint primitives = 0; glGetQueryObjectuiv(query, GL_QUERY_RESULT, &primitives);
        glDeleteQueries(1, &query);
        assert(primitives == count * 2);
        const auto moved = pixels(); assert(moved != birth);
        auto path = "/tmp/particles-" + std::to_string(count) + ".ppm";
        capture(path.c_str(),640,480);
        system.clear(); emitter.reset(); emitter.burst({0,0,0}, {0,1,0}); system.update(0.4f);
        draw(); assert(pixels() == moved);
        system.update(2); draw(); assert(system.size() == 0 && pixels() == empty);
    }
    // Depth occlusion and restoration of caller state.
    config.count = 100; assert(emitter.configure(config)); emitter.burst({0,0,0},{0,1,0});
    glClearDepth(0); glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); glClearDepth(1);
    glDisable(GL_DEPTH_TEST); glEnable(GL_BLEND); glDepthMask(GL_FALSE);
    glBlendFunc(GL_ONE, GL_ZERO);
    system.render(view, projection);
    assert(pixels() == empty);
    GLboolean mask; glGetBooleanv(GL_DEPTH_WRITEMASK, &mask);
    GLint source; glGetIntegerv(GL_BLEND_SRC_RGB, &source);
    assert(!glIsEnabled(GL_DEPTH_TEST) && glIsEnabled(GL_BLEND) && !mask && source == GL_ONE);
    glEnable(GL_DEPTH_TEST); glDisable(GL_BLEND); glDepthMask(GL_TRUE);
    system.clear(); system.releaseGraphics(); system.releaseGraphics();
    assert(!system.graphicsReady()); draw(); assert(pixels() == empty);
    assert(system.initGraphics());
}

static void checkTrails(MeteorSystem& meteors, ParticleSystem& particles)
{
    meteors.clear(); particles.clear();
    MeteorTrailEmitter trails(particles);
    Renderer renderer; LightManager lights;
    const auto view = glm::lookAt(glm::vec3(0,0,20),glm::vec3(0),glm::vec3(0,1,0));
    const auto projection = glm::ortho(-5.0f,5.0f,-3.75f,3.75f,0.1f,100.0f);
    auto draw = [&]() {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        meteors.render(renderer,lights,view,projection,{0,0,20});
    };
    std::vector<unsigned char> first;
    for (int pass = 0; pass < 2; ++pass) {
        meteors.clear(); particles.clear(); trails.reset();
        Transform meteor; meteor.position = {-2,0,0}; meteor.scale = glm::vec3(0.2f);
        assert(meteors.spawn(meteor,{4,1,0},2));
        trails.observe(meteors.meteors());
        for (int frame = 0; frame < 30; ++frame) {
            meteors.update(1.0f/60); particles.update(1.0f/60);
            trails.update(meteors.meteors(),1.0f/60);
        }
        assert(particles.size() > 10 && trails.trackedCount() == 1);
        draw(); const auto without = pixels();
        particles.render(view,projection); const auto with = pixels();
        assert(with != without);
        if (!pass) { first = with; capture("/tmp/meteor-trail.ppm",640,480); }
        else assert(with == first);
        meteors.update(3); particles.update(3); trails.update(meteors.meteors(),3);
        assert(meteors.size() == 0 && particles.size() == 0 && trails.trackedCount() == 0);
    }
}

static void checkEarthDamage()
{
    SceneResources resources; Scene scene; LightManager lights; Renderer renderer;
    assert(SceneSetup::build(scene,lights,resources));
    auto* earth = scene.findObject("Earth");
    assert(earth->material.textures.at("damageMap").texture == &resources.earthDamageTexture);
    EarthDamageSystem damage;
    assert(damage.upload(resources.earthDamageTexture) && !damage.dirty());
    DirectionalLight sun; sun.direction = {0,0,-1}; lights.setDirectionalLight(sun);
    const auto eye = earth->transform.position + glm::vec3(0,0,30);
    const auto view = glm::lookAt(eye,earth->transform.position,glm::vec3(0,1,0));
    const auto projection = glm::ortho(-13.0f,13.0f,-9.75f,9.75f,0.1f,100.0f);
    auto draw = [&]() {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        renderer.renderMesh(*earth->mesh,earth->material,earth->transform,lights,view,projection,eye);
    };
    auto readMap = [&]() {
        std::vector<float> result(EarthDamageSystem::Width * EarthDamageSystem::Height);
        resources.earthDamageTexture.bind(4);
        glGetTexImage(GL_TEXTURE_2D,0,GL_RED,GL_FLOAT,result.data());
        GLint wrapS,wrapT; glGetTexParameteriv(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,&wrapS);
        glGetTexParameteriv(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,&wrapT);
        assert(wrapS == GL_REPEAT && wrapT == GL_CLAMP_TO_EDGE);
        resources.earthDamageTexture.unbind(4);
        assert(glGetError() == GL_NO_ERROR);
        return result;
    };
    draw(); const auto baseline = pixels(); capture("/tmp/earth-damage-before.ppm",640,480);
    MeteorImpact impact{earth->transform.position + glm::vec3(0,0,10),{0,0,1},{0,0,-10},1};
    damage.consume({impact},earth->transform);
    assert(damage.dirty() && damage.upload(resources.earthDamageTexture));
    auto uploaded = readMap();
    for (std::size_t i = 0; i < uploaded.size(); ++i)
        assert(std::abs(uploaded[i]-damage.pixels()[i]) <= 1.0f/255);
    draw(); const auto burned = pixels(); assert(burned != baseline);
    long gain = 0; for (std::size_t i = 0; i < burned.size(); ++i) gain += int(burned[i])-int(baseline[i]);
    assert(gain < 0); capture("/tmp/earth-damage-burned.ppm",640,480);
    assert(damage.upload(resources.earthDamageTexture)); // Clean map remains unchanged.
    draw(); assert(pixels() == burned);
    damage.consume({impact},earth->transform); assert(damage.upload(resources.earthDamageTexture));
    draw(); const auto accumulated = pixels();
    long accumulatedGain = 0;
    for (std::size_t i = 0; i < accumulated.size(); ++i) accumulatedGain += int(accumulated[i])-int(burned[i]);
    assert(accumulatedGain < 0); capture("/tmp/earth-damage-accumulated.ppm",640,480);
    const auto attachedMap = readMap();
    // Rotate the local +Z mark onto the hidden back hemisphere.
    earth->transform.rotation.y = glm::pi<float>();
    draw(); const auto rotatedBurn = pixels();
    assert(readMap() == attachedMap); // Texture coordinates, not world projection.
    damage.clear(); assert(damage.upload(resources.earthDamageTexture));
    draw(); assert(pixels() == rotatedBurn); // Hidden mark no longer dims the front.
    earth->transform.rotation.y = 0;
    draw(); assert(pixels() == baseline);
    for (float value : readMap()) assert(value == 0);
    damage.consume({impact},earth->transform); assert(damage.upload(resources.earthDamageTexture));
    draw(); assert(pixels() == burned); // Reset and exact rendered replay.
    // Real texture seam: both edges receive the same footprint.
    damage.clear(); impact.position = earth->transform.position + glm::vec3(10,0,0);
    damage.consume({impact},earth->transform); assert(damage.upload(resources.earthDamageTexture));
    uploaded = readMap();
    const int middle = (EarthDamageSystem::Height/2)*EarthDamageSystem::Width;
    assert(uploaded[middle] > 0.6f && uploaded[middle+EarthDamageSystem::Width-1] > 0.6f);
}

static void checkEarthHeat()
{
    SceneResources resources; Scene scene; LightManager lights; Renderer renderer;
    assert(SceneSetup::build(scene,lights,resources));
    auto* earth = scene.findObject("Earth");
    assert(earth->material.textures.at("heatMap").texture == &resources.earthHeatTexture);
    DirectionalLight sun; sun.direction = {0,0,1}; sun.intensity = 0;
    lights.setDirectionalLight(sun); // Night side, no point lights or impact flash.
    EarthDamageSystem damage;
    const auto eye = earth->transform.position + glm::vec3(0,0,30);
    const auto view = glm::lookAt(eye,earth->transform.position,glm::vec3(0,1,0));
    const auto projection = glm::ortho(-13.0f,13.0f,-9.75f,9.75f,0.1f,100.0f);
    auto draw = [&]() {
        assert(damage.upload(resources.earthDamageTexture,resources.earthHeatTexture));
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        renderer.renderMesh(*earth->mesh,earth->material,earth->transform,lights,view,projection,eye);
        return pixels();
    };
    const auto baseline = draw();
    auto gain = [&](const std::vector<unsigned char>& image) {
        long sum = 0; for (std::size_t i = 0; i < image.size(); ++i) sum += int(image[i])-int(baseline[i]);
        return sum;
    };
    MeteorImpact impact{earth->transform.position+glm::vec3(0,0,10),{0,0,1},{0,0,-10},0.5f};
    damage.consume({impact},earth->transform);
    const auto peak = draw(); assert(gain(peak) > 0);
    capture("/tmp/earth-heat-night-peak.ppm",640,480);
    const int center = (240*640+320)*3;
    assert(peak[center] > 240 && peak[center+1] > 240); // Hot yellow-white center.
    resources.earthHeatTexture.bind(5);
    GLint format = 0; glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_INTERNAL_FORMAT,&format);
    assert(format == GL_R32F);
    std::vector<float> uploaded(damage.heatPixels().size());
    glGetTexImage(GL_TEXTURE_2D,0,GL_RED,GL_FLOAT,uploaded.data());
    assert(uploaded == damage.heatPixels()); // Values >1 survive upload without clipping.
    resources.earthHeatTexture.unbind(5);
    const auto permanent = damage.pixels();
    damage.update(1.5f);
    const auto cooled = draw(); assert(gain(cooled) > 0 && gain(cooled) < gain(peak));
    capture("/tmp/earth-heat-night-cooled.ppm",640,480);
    damage.consume({impact},earth->transform);
    const auto reheated = draw(); assert(gain(reheated) > gain(cooled));
    capture("/tmp/earth-heat-night-reheated.ppm",640,480);
    // Heat and damage remain attached when the affected hemisphere rotates away.
    const auto attachedHeat = damage.heatPixels();
    earth->transform.rotation.y = glm::pi<float>();
    const auto hidden = draw(); assert(damage.heatPixels() == attachedHeat);
    damage.clear(); assert(draw() == hidden);
    earth->transform.rotation.y = 0;
    assert(draw() == baseline);
    damage.consume({impact},earth->transform); assert(draw() == peak); // Reset image replay.
    damage.update(30); const auto cold = draw();
    assert(damage.pixels() == permanent && cold != peak);
    for (float heat : damage.heatPixels()) assert(heat == 0);
    // Once cold, burn remains visible under daylight.
    sun.direction = {0,0,-1}; sun.intensity = 1; lights.setDirectionalLight(sun);
    const auto coldDay = draw(); capture("/tmp/earth-heat-cold-burn.ppm",640,480);
    damage.clear(); const auto cleanDay = draw();
    long difference = 0;
    for (std::size_t i = 0; i < coldDay.size(); ++i) difference += int(coldDay[i])-int(cleanDay[i]);
    assert(difference < 0);
    resources.earthHeatTexture.bind(5);
    glGetTexImage(GL_TEXTURE_2D,0,GL_RED,GL_FLOAT,uploaded.data());
    for (float heat : uploaded) assert(heat == 0);
    resources.earthHeatTexture.unbind(5);
    assert(glGetError() == GL_NO_ERROR);
}

static void checkEarthCracks()
{
    SceneResources resources; Scene scene; LightManager lights; Renderer renderer;
    assert(SceneSetup::build(scene, lights, resources));
    auto* earth = scene.findObject("Earth");
    DirectionalLight sun; sun.direction = {0,0,1}; sun.intensity = 0;
    lights.setDirectionalLight(sun);
    glm::vec3 eye = earth->transform.position + glm::vec3(0,0,30);
    glm::mat4 view = glm::lookAt(eye, earth->transform.position, glm::vec3(0,1,0));
    const auto projection = glm::ortho(-13.0f,13.0f,-9.75f,9.75f,0.1f,100.0f);
    auto draw = [&](float level) {
        earth->material.setFloat("destructionLevel", level);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        renderer.renderMesh(*earth->mesh,earth->material,earth->transform,lights,view,projection,eye);
        return pixels();
    };
    const auto baseline = draw(0);
    assert(draw(0.02f) == baseline); // No cracks before damage or global progression.
    std::vector<std::vector<unsigned char>> firstPass;
    long previousGain = 0;
    std::size_t previousArea = 0;
    for (float level : {0.2f,0.5f,0.8f,0.95f}) {
        const auto image = draw(level);
        long gain = 0; std::size_t area = 0;
        for (std::size_t i = 0; i < image.size(); i += 3) {
            int difference = int(image[i])+int(image[i+1])+int(image[i+2])
                -int(baseline[i])-int(baseline[i+1])-int(baseline[i+2]);
            gain += difference;
            if (difference > 40) ++area;
        }
        assert(gain >= previousGain && area >= previousArea);
        previousGain = gain; previousArea = area;
        firstPass.push_back(image);
        auto path = "/tmp/earth-cracks-" + std::to_string(int(level*100)) + ".ppm";
        capture(path.c_str(),640,480);
    }
    assert(previousGain > 100000 && previousArea > 1000);
    const auto& extreme = firstPass.back();
    bool whiteInterior = false;
    for (std::size_t i = 0; i < extreme.size(); i += 3)
        if (extreme[i] > 240 && extreme[i+1] > 240 && extreme[i+2] > 200) whiteInterior = true;
    assert(whiteInterior); // Extreme emission with no Sun or point lights.
    // The depth silhouette is identical: no vertex displacement or breakup.
    std::vector<float> intactDepth(640*480), crackedDepth(640*480);
    draw(0); glReadPixels(0,0,640,480,GL_DEPTH_COMPONENT,GL_FLOAT,intactDepth.data());
    draw(0.95f); glReadPixels(0,0,640,480,GL_DEPTH_COMPONENT,GL_FLOAT,crackedDepth.data());
    assert(intactDepth == crackedDepth);
    // Rotate Earth and view together: local patterns remain registered to the crust.
    earth->transform.rotation.y = 0.7f;
    const auto rotation = glm::rotate(glm::mat4(1),0.7f,glm::vec3(0,1,0));
    eye = earth->transform.position + glm::vec3(rotation*glm::vec4(0,0,30,0));
    view = glm::lookAt(eye,earth->transform.position,glm::vec3(0,1,0));
    sun.direction = glm::vec3(rotation*glm::vec4(0,0,1,0)); lights.setDirectionalLight(sun);
    const auto rotated = draw(0.95f);
    long error = 0;
    for (std::size_t i = 0; i < extreme.size(); ++i) error += std::abs(int(extreme[i])-int(rotated[i]));
    assert(double(error)/extreme.size() < 1.0); // Float transform/interpolation tolerance.
    earth->transform.rotation.y = 0;
    eye = earth->transform.position + glm::vec3(0,0,30);
    view = glm::lookAt(eye,earth->transform.position,glm::vec3(0,1,0));
    sun.direction = {0,0,1}; lights.setDirectionalLight(sun);
    assert(draw(0) == baseline);
    int index = 0;
    for (float level : {0.2f,0.5f,0.8f,0.95f}) assert(draw(level) == firstPass[index++]);
    // At early levels, a cooled damage patch nucleates fractures locally.
    EarthDamageSystem damage;
    MeteorImpact impact{earth->transform.position+glm::vec3(0,0,10),{0,0,1},{0,0,-10},1};
    damage.consume({impact},earth->transform); damage.update(30);
    assert(damage.upload(resources.earthDamageTexture,resources.earthHeatTexture));
    const auto burned = draw(0);
    const auto early = draw(0.1f);
    assert(early != burned);
    long earlyGain = 0;
    for (std::size_t i = 0; i < early.size(); ++i) earlyGain += int(early[i])-int(burned[i]);
    assert(earlyGain > 0);
    capture("/tmp/earth-cracks-local.ppm",640,480);
    // Ongoing thermal impacts still add emission on top of the crack network.
    damage.consume({impact},earth->transform);
    assert(damage.upload(resources.earthDamageTexture,resources.earthHeatTexture));
    const auto hot = draw(0.1f); assert(hot != early);
    damage.clear(); assert(damage.upload(resources.earthDamageTexture,resources.earthHeatTexture));
    assert(draw(0) == baseline);
    assert(glGetError() == GL_NO_ERROR);
}

static void checkEarthBreakup()
{
    SceneResources resources; Scene scene; LightManager lights; Renderer renderer;
    assert(SceneSetup::build(scene,lights,resources));
    auto* earth=scene.findObject("Earth");
    EarthBreakupSystem breakup; assert(breakup.initGraphics(resources.earthSphere.getMesh()));
    assert(breakup.initGraphics(resources.earthSphere.getMesh()));
    HDRPipeline hdr; assert(hdr.init(640,480));
    const auto eye=earth->transform.position+glm::vec3(0,0,45);
    const auto view=glm::lookAt(eye,earth->transform.position,glm::vec3(0,1,0));
    const auto projection=glm::ortho(-24.0f,24.0f,-18.0f,18.0f,0.1f,150.0f);
    auto draw=[&](bool post, bool bloom=true) {
        if(post) assert(hdr.begin(640,480));
        else glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        if(breakup.active()) breakup.render(renderer,earth->material,lights,view,projection,eye);
        else renderer.renderMesh(*earth->mesh,earth->material,earth->transform,lights,view,projection,eye);
        if(post) hdr.finish(bloom);
        return pixels();
    };
    const auto baseline=draw(false);
    std::vector<float> baselineDepth(640*480), fragmentDepth(640*480);
    glReadPixels(0,0,640,480,GL_DEPTH_COMPONENT,GL_FLOAT,baselineDepth.data());
    std::vector<unsigned char> replay;
    for(int pass=0;pass<2;++pass) {
        breakup.reset(); lights.setTransientPointLights({});
        assert(draw(false)==baseline);
        breakup.update(0.1f,1,earth->transform);
        draw(false); glReadPixels(0,0,640,480,GL_DEPTH_COMPONENT,GL_FLOAT,fragmentDepth.data());
        // At birth, all exterior triangles reconstruct the existing sphere.
        double depthError=0;
        for(std::size_t i=0;i<baselineDepth.size();++i) depthError+=std::abs(baselineDepth[i]-fragmentDepth[i]);
        assert(depthError/baselineDepth.size()<0.00001);
        breakup.publish(lights);
        breakup.update(1.5f,1,earth->transform);
        const auto separated=draw(false); assert(separated!=baseline);
        if(!pass) capture("/tmp/earth-breakup-fragments.ppm",640,480);
        // Read HDR before composition: the core retains true values above one.
        assert(hdr.begin(640,480));
        breakup.render(renderer,earth->material,lights,view,projection,eye);
        std::vector<float> raw(640*480*3);
        glReadPixels(0,0,640,480,GL_RGB,GL_FLOAT,raw.data());
        assert(*std::max_element(raw.begin(),raw.end())>10);
        hdr.finish(false); const auto noBloom=pixels();
        const auto withBloom=draw(true,true); assert(withBloom!=noBloom);
        long gain=0; std::size_t bright=0;
        for(std::size_t i=0;i<withBloom.size();++i) { gain+=int(withBloom[i])-int(noBloom[i]); if(withBloom[i]>245)++bright; }
        assert(gain>0 && bright<withBloom.size()/2); // Glow does not white out the frame.
        if(!pass) { replay=withBloom; capture("/tmp/earth-breakup-bloom.ppm",640,480); }
        else assert(withBloom==replay);
        // Removing the core light changes actual fragment shading.
        lights.setTransientPointLights({}); const auto unlit=draw(true);
        assert(unlit!=withBloom);
        breakup.update(1.5f,1,earth->transform); breakup.publish(lights);
        assert(draw(true)!=withBloom);
        if(!pass) capture("/tmp/earth-breakup-core-exposed.ppm",640,480);
    }
    // Resize the HDR buffers, then restore; all FBOs must remain complete.
    assert(hdr.begin(320,240)); hdr.finish();
    breakup.reset(); lights.setTransientPointLights({}); draw(true);
    breakup.releaseGraphics(); breakup.releaseGraphics(); assert(!breakup.graphicsReady());
    assert(breakup.initGraphics(resources.earthSphere.getMesh()));
    assert(glGetError()==GL_NO_ERROR);
}

static void checkHDRColorAndSun()
{
    HDRPipeline hdr; assert(hdr.init(640,480));
    GLfloat previousClear[4]; glGetFloatv(GL_COLOR_CLEAR_VALUE,previousClear);
    // Regression: ordinary display colors must survive post-processing without
    // the gamma lift that previously turned a dark background grey.
    for(const glm::vec3 sample : {glm::vec3(0.02f),glm::vec3(0.06f),glm::vec3(0.1f,0.3f,0.6f)}) {
        glClearColor(sample.r,sample.g,sample.b,1);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        const auto direct=pixels();
        assert(hdr.begin(640,480)); hdr.finish(false);
        const auto composed=pixels();
        for(std::size_t i=0;i<direct.size();++i) assert(std::abs(int(direct[i])-int(composed[i]))<=1);
    }
    SceneResources resources; Scene scene; LightManager lights; Renderer renderer;
    assert(SceneSetup::build(scene,lights,resources));
    auto* earth=scene.findObject("Earth");
    auto eye=earth->transform.position+glm::vec3(0,0,30);
    auto view=glm::lookAt(eye,earth->transform.position,glm::vec3(0,1,0));
    auto projection=glm::ortho(-13.0f,13.0f,-9.75f,9.75f,0.1f,100.0f);
    glClearColor(0,0,0,1); glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    renderer.renderMesh(*earth->mesh,earth->material,earth->transform,lights,view,projection,eye);
    const auto legacy=pixels(); capture("/tmp/earth-color-legacy.ppm",640,480);
    assert(hdr.begin(640,480));
    renderer.renderMesh(*earth->mesh,earth->material,earth->transform,lights,view,projection,eye);
    hdr.finish(false); const auto corrected=pixels();
    capture("/tmp/earth-color-corrected.ppm",640,480);
    double difference=0;
    for(std::size_t i=0;i<legacy.size();++i) difference+=std::abs(int(legacy[i])-int(corrected[i]));
    assert(difference/legacy.size()<2); // Legacy Earth appearance, gentle highlight compression.
    auto* sun=scene.findObject("Sun");
    eye=sun->transform.position+glm::vec3(0,0,180);
    view=glm::lookAt(eye,sun->transform.position,glm::vec3(0,1,0));
    projection=glm::ortho(-85.0f,85.0f,-63.75f,63.75f,0.1f,300.0f);
    auto draw=[&](bool bloom) {
        assert(hdr.begin(640,480));
        renderer.renderMesh(*sun->mesh,sun->material,sun->transform,lights,view,projection,eye);
        hdr.finish(bloom); return pixels();
    };
    const auto without=draw(false); capture("/tmp/sun-without-halo.ppm",640,480);
    const auto with=draw(true); capture("/tmp/sun-with-halo.ppm",640,480);
    const float solarBloom=sun->material.floatUniforms.at("bloomEmission");
    sun->material.setFloat("bloomEmission",0);
    assert(draw(false)==without); // Halo strength never changes the visible solar surface.
    const auto ordinaryBloom=draw(true);
    sun->material.setFloat("bloomEmission",solarBloom);
    long extraHalo=0;
    for(std::size_t i=0;i<with.size();++i) extraHalo+=int(with[i])-int(ordinaryBloom[i]);
    assert(extraHalo>0); // Separate source adds glow at the configured solar intensity.
    std::size_t haloPixels=0; long gain=0;
    for(std::size_t i=0;i<with.size();i+=3) {
        gain+=int(with[i])+int(with[i+1])+int(with[i+2])-int(without[i])-int(without[i+1])-int(without[i+2]);
        if(without[i]==0 && without[i+1]==0 && without[i+2]==0 && int(with[i])+int(with[i+1])+int(with[i+2])>10) ++haloPixels;
    }
    assert(gain>0 && haloPixels>100);
    assert(with[0]==0 && with[1]==0 && with[2]==0); // Halo remains local, background stays black.
    assert(draw(true)==with); // No temporal glow history or nondeterminism.
    // A foreground object masks the separate source using the shared depth buffer.
    const auto originalEarth=earth->transform;
    earth->transform.position=sun->transform.position+glm::vec3(0,0,70);
    assert(hdr.begin(640,480));
    renderer.renderMesh(*sun->mesh,sun->material,sun->transform,lights,view,projection,eye);
    renderer.renderMesh(*earth->mesh,earth->material,earth->transform,lights,view,projection,eye);
    glReadBuffer(GL_COLOR_ATTACHMENT1);
    GLfloat occluded[3]; glReadPixels(320,240,1,1,GL_RGB,GL_FLOAT,occluded);
    assert(occluded[0]==0 && occluded[1]==0 && occluded[2]==0);
    glReadBuffer(GL_COLOR_ATTACHMENT0); hdr.finish();
    earth->transform=originalEarth;
    glClearColor(previousClear[0],previousClear[1],previousClear[2],previousClear[3]);
    assert(glGetError()==GL_NO_ERROR);
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
    checkParticles(app.particles());
    checkTrails(app.meteors(),app.particles());
    checkEarthDamage();
    checkEarthHeat();
    checkEarthCracks();
    checkEarthBreakup();
    checkHDRColorAndSun();
    Transform meteor;
    meteor.position = {30, 50, 15};
    meteor.scale = glm::vec3(0.5f);
    assert(app.meteors().spawn(meteor, {1, 0, 0}, 100));
    app.run(3); // Also exercise Application's simulation and render integration.
    assert(app.meteors().size() == 1);
    assert(app.meteors().meteors()[0].transform.position.x > 30);
    Transform touching;
    touching.position = {30, 50, 10.4f};
    touching.scale = glm::vec3(0.5f);
    assert(app.meteors().spawn(touching, {0, 0, -10}, 5));
    app.run(1);
    assert(app.impactLights().lights().size() == 1);
    assert(app.earthDamage().destructionLevel() > 0);
    assert(*std::max_element(app.earthDamage().heatPixels().begin(),app.earthDamage().heatPixels().end()) > 0);
    assert(*std::max_element(app.earthDamage().pixels().begin(),app.earthDamage().pixels().end()) > 0);
    assert(app.particles().size() >= 48 && app.particles().particles().back().age == 0);
    assert(app.impactLights().lights()[0].intensity == app.impactLights().lights()[0].initialIntensity);
    app.restartSequence();
    assert(app.meteors().size() == 0);
    assert(app.impactLights().lights().empty());
    assert(app.particles().size() == 0);
    assert(app.earthDamage().destructionLevel() == 0 && !app.earthBreakup().active());
    for (float value : app.earthDamage().pixels()) assert(value == 0);
    for (float value : app.earthDamage().heatPixels()) assert(value == 0);
    app.run(3);
    assert(app.meteors().size() == 0);
    Transform catastrophic;
    catastrophic.position={30,50,10.4f}; catastrophic.scale=glm::vec3(1);
    assert(app.meteors().spawn(catastrophic,{0,0,-100},5));
    app.run(1);
    assert(app.earthBreakup().active() && app.earthDamage().destructionLevel()==1);
    app.run(3);
    app.restartSequence();
    assert(!app.earthBreakup().active() && app.earthDamage().destructionLevel()==0);
    app.run(1);
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
        const auto collider = SceneSetup::earthCollider(scene, resources);
        assert(collider.center == scene.findObject("Earth")->transform.position);
        assert(collider.radius == resources.earthSphere.getRadius() * scene.findObject("Earth")->transform.scale.x);
        auto* earth = scene.findObject("Earth");
        earth->transform.scale = glm::vec3(20);
        assert(SceneSetup::earthCollider(scene, resources).radius == resources.earthSphere.getRadius() * 20);
        earth->transform.scale.y = 21;
        bool rejectedScale = false;
        try { SceneSetup::earthCollider(scene, resources); }
        catch (const std::invalid_argument&) { rejectedScale = true; }
        assert(rejectedScale);
        earth->transform.scale = glm::vec3(10);
        MeteorSystem meteors;
        ImpactLightSystem flashes;
        ParticleSystem particles;
        ImpactParticleEmitter particleEmitter(particles);
        MeteorTrailEmitter trails(particles);
        EarthDamageSystem damage;
        EarthBreakupSystem breakup;
        assert(breakup.initGraphics(resources.earthSphere.getMesh()));
        HDRPipeline hdr; assert(hdr.init(640,480));
        assert(particles.initGraphics());
        MeteorShower shower(meteors);
        assert(meteors.initGraphics());
        assert(MainSequence::build(timeline, camera, scene, shower));
        const int frames[] = {0, 96, 160, 192, 208, 224, 264, 360};
        constexpr int imageCount = sizeof(frames)/sizeof(frames[0]);
        const char* images[] = {"/tmp/space-start.ppm", "/tmp/space-middle.ppm",
            "/tmp/space-bombardment.ppm", "/tmp/space-cracks.ppm",
            "/tmp/space-breakup.ppm", "/tmp/space-core.ppm",
            "/tmp/space-fragments.ppm", "/tmp/space-end.ppm"};
        std::vector<std::vector<unsigned char>> firstPass;
        std::vector<std::vector<MeteorImpact>> firstImpacts;
        std::vector<std::vector<ImpactLight>> firstLights;
        std::vector<std::vector<Particle>> firstParticles;
        std::vector<std::vector<float>> firstDamage, firstHeat;
        std::vector<float> firstDestruction;
        for (int pass = 0; pass < 2; ++pass)
        {
            MainSequence::reset(timeline, shower, meteors);
            flashes.clear();
            breakup.reset(); scene.findObject("Earth")->visible=true; scene.findObject("EarthClouds")->visible=true;
            damage.clear(); assert(damage.destructionLevel() == 0); assert(damage.upload(resources.earthDamageTexture, resources.earthHeatTexture));
            particles.clear(); particleEmitter.reset(); trails.reset();
            flashes.publish(lights);
            timeline.play();
            int imageIndex = 0;
            std::size_t impactCount = 0;
            float previousDestruction = 0;
            for (int frame = 0; frame <= 360; ++frame)
            {
                if (frame > 0)
                {
                    timeline.update(0.25f);
                    trails.observe(meteors.meteors());
                    if (breakup.active()) meteors.update(0.25f);
                    else meteors.update(0.25f, SceneSetup::earthCollider(scene, resources));
                    particles.update(0.25f);
                    trails.update(meteors.meteors(), 0.25f);
                    damage.update(0.25f);
                    damage.consume(meteors.impacts(),scene.findObject("Earth")->transform);
                    assert(damage.upload(resources.earthDamageTexture, resources.earthHeatTexture));
                    breakup.update(0.25f,damage.destructionLevel(),scene.findObject("Earth")->transform);
                    particleEmitter.consume(meteors.impacts());
                    flashes.consume(meteors.impacts());
                    flashes.update(0.25f);
                    flashes.publish(lights);
                    breakup.publish(lights);
                    if (breakup.active()) shower.stop();
                    shower.update(0.25f);
                }
                assert(damage.destructionLevel() >= previousDestruction && damage.destructionLevel() <= 1);
                previousDestruction = damage.destructionLevel();
                if (pass == 0) firstDestruction.push_back(damage.destructionLevel());
                else assert(damage.destructionLevel() == firstDestruction[frame]);
                impactCount += meteors.impacts().size();
                for (const auto& impact : meteors.impacts())
                {
                    assert(std::abs(glm::length(impact.position - collider.center) - collider.radius) < 0.0001f);
                    assert(std::abs(glm::length(impact.normal) - 1) < 0.0001f);
                }
                if (!breakup.active()) for (const auto& meteor : meteors.meteors())
                    assert(glm::distance(meteor.transform.position, collider.center)
                        >= collider.radius + meteor.transform.scale.x - 0.0001f);
                if (pass == 0) firstImpacts.push_back(meteors.impacts());
                else
                {
                    assert(meteors.impacts().size() == firstImpacts[frame].size());
                    for (std::size_t i = 0; i < meteors.impacts().size(); ++i)
                    {
                        const auto& a = meteors.impacts()[i];
                        const auto& b = firstImpacts[frame][i];
                        assert(a.position == b.position && a.normal == b.normal);
                        assert(a.velocity == b.velocity && a.meteorScale == b.meteorScale);
                    }
                }
                if (pass == 0) { firstDamage.push_back(damage.pixels()); firstHeat.push_back(damage.heatPixels()); }
                else { assert(damage.pixels() == firstDamage[frame]); assert(damage.heatPixels() == firstHeat[frame]); }
                if (pass == 0) firstParticles.push_back(particles.particles());
                else {
                    assert(particles.size() == firstParticles[frame].size());
                    for (std::size_t i = 0; i < particles.size(); ++i) {
                        const auto& a = particles.particles()[i]; const auto& b = firstParticles[frame][i];
                        assert(a.position == b.position && a.velocity == b.velocity);
                        assert(a.size == b.size && a.age == b.age && a.lifetime == b.lifetime);
                    }
                }
                if (pass == 0) firstLights.push_back(flashes.lights());
                else
                {
                    assert(flashes.lights().size() == firstLights[frame].size());
                    for (std::size_t i = 0; i < flashes.lights().size(); ++i)
                    {
                        const auto& a = flashes.lights()[i];
                        const auto& b = firstLights[frame][i];
                        assert(a.position == b.position && a.color == b.color);
                        assert(a.initialIntensity == b.initialIntensity && a.intensity == b.intensity);
                        assert(a.age == b.age && a.lifetime == b.lifetime);
                    }
                }
                if (frame < 32) assert(meteors.size() == 0 && !shower.isRunning());
                if (frame == 96) assert(meteors.size() > 0 && shower.isRunning());
                if (frame >= 248) assert(!shower.isRunning());
                if (frame == 360) assert(meteors.size() == 0 && breakup.active());
                if (imageIndex >= imageCount || frame != frames[imageIndex]) continue;
                SceneSetup::update(scene, camera.getPosition());
                assert(scene.findObject("Stars")->transform.position == camera.getPosition());
                assert(hdr.begin(640,480));
                const auto view = camera.getViewMatrix();
                const auto projection = glm::perspective(glm::radians(camera.getFOV()),
                    640.0f / 480, 0.1f, 100000000.0f);
                scene.findObject("Earth")->visible=!breakup.active();
                scene.findObject("EarthClouds")->visible=!breakup.active();
                scene.findObject("Earth")->material.setFloat("destructionLevel", damage.destructionLevel());
                renderer.render(scene, lights, view, projection, camera.getPosition());
                const auto withoutMeteors = pixels();
                meteors.render(renderer, lights, view, projection, camera.getPosition());
                const auto rendered = pixels();
                if (meteors.size() == 0) assert(rendered == withoutMeteors);
                // Meteors can exist outside the camera: offscreen births are intentional.
                breakup.render(renderer,scene.findObject("Earth")->material,lights,view,projection,camera.getPosition());
                particles.render(view, projection);
                hdr.finish();
                const auto withParticles = pixels();
                if (pass == 0)
                {
                    capture(images[imageIndex], 640, 480);
                    firstPass.push_back(withParticles);
                }
                else assert(withParticles == firstPass[imageIndex]); // Actual image replay.
                ++imageIndex;
            }
            assert(imageIndex == imageCount && !timeline.isPlaying() && timeline.getTime() == MainSequence::Duration);
            assert(impactCount > 0 && damage.destructionLevel() > 0 && breakup.active());
        }
        breakup.reset(); scene.findObject("Earth")->visible=true; scene.findObject("EarthClouds")->visible=true;
        damage.clear(); assert(damage.upload(resources.earthDamageTexture, resources.earthHeatTexture));
        scene.findObject("Earth")->material.setFloat("destructionLevel", 0.0f);
        // Controlled visible contact in front of the rendered Earth.
        MainSequence::reset(timeline, shower, meteors);
        flashes.clear();
        flashes.publish(lights);
        assert(meteors.impacts().empty());
        SceneSetup::update(scene, camera.getPosition());
        const auto view = camera.getViewMatrix();
        const auto projection = glm::perspective(glm::radians(camera.getFOV()),
            640.0f / 480, 0.1f, 100000000.0f);
        auto drawContact = [&]()
        {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            renderer.render(scene, lights, view, projection, camera.getPosition());
            meteors.render(renderer, lights, view, projection, camera.getPosition());
        };
        drawContact();
        const auto baseline = pixels();
        Transform approaching;
        approaching.position = collider.center + glm::vec3(0, 0, collider.radius + 3);
        approaching.scale = glm::vec3(0.5f);
        assert(meteors.spawn(approaching, {0, 0, -10}, 5));
        drawContact();
        assert(pixels() != baseline);
        capture("/tmp/impact-before.ppm", 640, 480);
        meteors.update(0.2f, collider);
        assert(meteors.size() == 1 && meteors.impacts().empty());
        drawContact();
        capture("/tmp/impact-approaching.ppm", 640, 480);
        meteors.update(0.1f, collider);
        assert(meteors.size() == 0 && meteors.impacts().size() == 1);
        assert(glm::length(meteors.impacts()[0].position
            - (collider.center + glm::vec3(0, 0, collider.radius))) < 0.0001f);
        drawContact();
        assert(pixels() == baseline); // Meteor gone before consuming its impact.
        capture("/tmp/impact-removed.ppm", 640, 480);
        flashes.consume(meteors.impacts());
        flashes.update(0.1f);
        flashes.publish(lights);
        drawContact();
        const auto peak = pixels();
        assert(peak != baseline); // Real Earth shading changes, no visible source mesh.
        capture("/tmp/impact-flash-peak.ppm", 640, 480);
        auto brightnessGain = [&](const std::vector<unsigned char>& image)
        {
            long gain = 0;
            for (std::size_t i = 0; i < image.size(); ++i)
                gain += int(image[i]) - int(baseline[i]);
            return gain;
        };
        assert(brightnessGain(peak) > 0);
        flashes.update(0.25f);
        flashes.publish(lights);
        drawContact();
        const auto faded = pixels();
        assert(brightnessGain(faded) > 0 && brightnessGain(faded) < brightnessGain(peak));
        capture("/tmp/impact-flash-faded.ppm", 640, 480);
        flashes.update(0.25f);
        flashes.publish(lights);
        drawContact();
        assert(flashes.lights().empty() && pixels() == baseline);
        capture("/tmp/impact-flash-expired.ppm", 640, 480);

        particles.clear(); particleEmitter.reset(); particleEmitter.consume(meteors.impacts());
        assert(particles.size() == 48);
        drawContact(); const auto withoutParticles = pixels();
        particles.render(view, projection); assert(pixels() != withoutParticles);
        capture("/tmp/impact-particles-birth.ppm", 640, 480);
        particles.update(0.4f); drawContact(); particles.render(view, projection);
        capture("/tmp/impact-particles-moved.ppm", 640, 480);
        particles.update(2); drawContact(); particles.render(view, projection);
        assert(particles.size() == 0 && pixels() == baseline);

        // Query the actual shader uniform after an overflowing impact burst.
        std::vector<MeteorImpact> burst(80, {{30, 50, 10}, {0, 0, 1}, {0, 0, -10}, 0.5f});
        flashes.consume(burst);
        flashes.update(1);
        flashes.publish(lights);
        resources.earthShader.use();
        lights.applyToShader(resources.earthShader);
        GLint count = 0;
        glGetUniformiv(resources.earthShader.getProgram(),
            resources.earthShader.getUniformLocation("pointLightCount"), &count);
        assert(count == int(LightManager::MaxPointLights));
        drawContact();
        assert(glGetError() == GL_NO_ERROR);
    } // All these GPU resources are released while the context is alive.
    assert(glGetError() == GL_NO_ERROR);
    app.shutdown();
    app.shutdown();
    assert(app.meteors().size() == 0 && !app.meteors().graphicsReady());
    assert(app.impactLights().lights().empty());
    assert(app.particles().size() == 0);
    assert(app.earthDamage().destructionLevel() == 0 && !app.earthBreakup().active());
    for (float value : app.earthDamage().pixels()) assert(value == 0);
    for (float value : app.earthDamage().heatPixels()) assert(value == 0);
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
    // All earlier resources loaded, particle shader absent: release partial init.
    std::filesystem::create_symlink(original / "shaders/meteor.frag", empty / "shaders/meteor.frag");
    std::filesystem::remove(empty / "shaders/particle.frag");
    assert(!app.init());
    assert(glfwGetCurrentContext() == nullptr);
    assert(!app.particles().graphicsReady() && !app.meteors().graphicsReady());
    // Missing breakup and post-processing shaders must also clean partial resources.
    std::filesystem::create_symlink(original / "shaders/particle.frag", empty / "shaders/particle.frag");
    std::filesystem::remove(empty / "shaders/core.frag");
    assert(!app.init() && glfwGetCurrentContext()==nullptr && !app.earthBreakup().graphicsReady());
    std::filesystem::create_symlink(original / "shaders/core.frag", empty / "shaders/core.frag");
    std::filesystem::remove(empty / "shaders/bright.frag");
    assert(!app.init() && glfwGetCurrentContext()==nullptr && !app.earthBreakup().graphicsReady());
    std::filesystem::current_path(original);
    std::filesystem::remove_all(empty);
    assert(app.init());
    app.run(3);
    std::cout << "Application lifecycle, Earth breakup, HDR bloom and cinematic image replay checks passed\n";
} // Application destructor also releases resources before GLFW.
