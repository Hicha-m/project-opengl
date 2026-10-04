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
    assert(*std::max_element(app.earthDamage().pixels().begin(),app.earthDamage().pixels().end()) > 0);
    assert(app.particles().size() >= 48 && app.particles().particles().back().age == 0);
    assert(app.impactLights().lights()[0].intensity == app.impactLights().lights()[0].initialIntensity);
    app.restartSequence();
    assert(app.meteors().size() == 0);
    assert(app.impactLights().lights().empty());
    assert(app.particles().size() == 0);
    for (float value : app.earthDamage().pixels()) assert(value == 0);
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
        assert(particles.initGraphics());
        MeteorShower shower(meteors);
        assert(meteors.initGraphics());
        assert(MainSequence::build(timeline, camera, scene, shower));
        const int frames[] = {0, 60, 80, 88, 108, 120};
        const char* images[] = {"/tmp/space-start.ppm", "/tmp/space-middle.ppm",
            "/tmp/space-shower-stop.ppm", "/tmp/space-after-shower.ppm",
            "/tmp/space-meteors-expired.ppm", "/tmp/space-end.ppm"};
        std::vector<std::vector<unsigned char>> firstPass;
        std::vector<std::vector<MeteorImpact>> firstImpacts;
        std::vector<std::vector<ImpactLight>> firstLights;
        std::vector<std::vector<Particle>> firstParticles;
        std::vector<std::vector<float>> firstDamage;
        for (int pass = 0; pass < 2; ++pass)
        {
            MainSequence::reset(timeline, shower, meteors);
            flashes.clear();
            damage.clear(); assert(damage.upload(resources.earthDamageTexture));
            particles.clear(); particleEmitter.reset(); trails.reset();
            flashes.publish(lights);
            timeline.play();
            int imageIndex = 0;
            std::size_t impactCount = 0;
            for (int frame = 0; frame <= 120; ++frame)
            {
                if (frame > 0)
                {
                    timeline.update(0.25f);
                    trails.observe(meteors.meteors());
                    meteors.update(0.25f, SceneSetup::earthCollider(scene, resources));
                    particles.update(0.25f);
                    trails.update(meteors.meteors(), 0.25f);
                    damage.consume(meteors.impacts(),scene.findObject("Earth")->transform);
                    assert(damage.upload(resources.earthDamageTexture));
                    particleEmitter.consume(meteors.impacts());
                    flashes.consume(meteors.impacts());
                    flashes.update(0.25f);
                    flashes.publish(lights);
                    shower.update(0.25f);
                }
                impactCount += meteors.impacts().size();
                for (const auto& impact : meteors.impacts())
                {
                    assert(std::abs(glm::length(impact.position - collider.center) - collider.radius) < 0.0001f);
                    assert(std::abs(glm::length(impact.normal) - 1) < 0.0001f);
                }
                for (const auto& meteor : meteors.meteors())
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
                if (pass == 0) firstDamage.push_back(damage.pixels());
                else assert(damage.pixels() == firstDamage[frame]);
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
                if (frame < 40) assert(meteors.size() == 0 && !shower.isRunning());
                if (frame == 60) assert(meteors.size() > 0 && shower.isRunning());
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
                particles.render(view, projection);
                const auto withParticles = pixels();
                if (pass == 0)
                {
                    capture(images[imageIndex], 640, 480);
                    firstPass.push_back(withParticles);
                }
                else assert(withParticles == firstPass[imageIndex]); // Actual image replay.
                ++imageIndex;
            }
            assert(imageIndex == 6 && !timeline.isPlaying() && timeline.getTime() == 30);
            assert(impactCount > 0);
        }
        damage.clear(); assert(damage.upload(resources.earthDamageTexture));
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
    for (float value : app.earthDamage().pixels()) assert(value == 0);
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
    std::filesystem::current_path(original);
    std::filesystem::remove_all(empty);
    assert(app.init());
    app.run(3);
    std::cout << "Application lifecycle, Earth damage, meteor trails, impacts and cinematic image replay checks passed\n";
} // Application destructor also releases resources before GLFW.
