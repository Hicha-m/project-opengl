#include "cinematic/MainSequence.h"
#include "animation/CameraTrack.h"
#include "animation/TransformTrack.h"
#include "animation/EventTrack.h"

bool MainSequence::build(Timeline& timeline, CinematicCamera& camera, Scene& scene, MeteorShower& shower)
{
    auto* earth = scene.findObject("Earth");
    if (!earth || !scene.findObject("EarthClouds")) return false;
    MeteorShowerConfig config;
    config.spawnRate = 30;
    config.origin = earth->transform.position + glm::vec3(0, 17, 0);
    config.spawnHalfExtents = {17, 3, 17};
    config.direction = {0.15f, -1, 0.1f};
    config.spreadRadians = glm::radians(12.0f);
    config.minSpeed = 3;
    config.maxSpeed = 5;
    config.minScale = 0.4f;
    config.maxScale = 0.9f;
    config.minLifetime = 4;
    config.maxLifetime = 7;
    config.seed = 42;
    if (!shower.configure(config)) return false;
    // A fresh sequence also supports Application shutdown/init on the same instance.
    camera.positionTrack() = {};
    camera.targetTrack() = {};
    camera.fovTrack() = {};
    camera.orbitTargetTrack() = {};
    camera.orbitRadiusTrack() = {};
    camera.orbitYawTrack() = {};
    camera.orbitPitchTrack() = {};
    camera.enableOrbit(true);
    camera.orbitTargetTrack().addKeyframe(0, earth->transform.position);
    camera.orbitTargetTrack().addKeyframe(30, earth->transform.position);
    auto easing = EasingType::EaseInOut;
    camera.orbitRadiusTrack().addKeyframe(0, 30, easing);
    camera.orbitRadiusTrack().addKeyframe(10, 40, easing);
    camera.orbitRadiusTrack().addKeyframe(20, 100, easing);
    camera.orbitRadiusTrack().addKeyframe(30, 300, easing);
    camera.orbitYawTrack().addKeyframe(0, 0, easing);
    camera.orbitYawTrack().addKeyframe(30, 180, easing);
    camera.orbitPitchTrack().addKeyframe(0, 10, easing);
    camera.orbitPitchTrack().addKeyframe(30, 25, easing);
    camera.fovTrack().addKeyframe(0, 45, easing);
    camera.fovTrack().addKeyframe(15, 55, easing);
    camera.fovTrack().addKeyframe(30, 65, easing);

    timeline.setDuration(30.0f);

    timeline.addTrack(std::make_unique<CameraTrack>(camera));

    auto animateRotation = [&](const std::string& name, float degreesPerSecond)
    {
        auto track = std::make_unique<TransformTrack>([&scene, name]() -> Transform*
        {
            auto* object = scene.findObject(name);
            return object ? &object->transform : nullptr;
        });
        track->rotationTrack().addKeyframe(0, glm::vec3(0));
        track->rotationTrack().addKeyframe(30, glm::vec3(0, glm::radians(30 * degreesPerSecond), 0));
        timeline.addTrack(std::move(track));
    };
    animateRotation("Earth", 100.0f);
    animateRotation("EarthClouds", 5.0f);

    auto events = std::make_unique<EventTrack>();
    events->addEvent(10, [&shower] { shower.start(); });
    events->addEvent(20, [&shower] { shower.stop(); });
    timeline.addTrack(std::move(events));

    timeline.play();

    return true;
}

void MainSequence::reset(Timeline& timeline, MeteorShower& shower, MeteorSystem& system)
{
    timeline.stop(); // Rearm events and reset camera/transform tracks.
    shower.reset();  // Restore the configured seed and spawn credit.
    system.clear();
}
