#include "cinematic/MainSequence.h"
#include "animation/CameraTrack.h"
#include "animation/TransformTrack.h"
#include "animation/EventTrack.h"
#include "systems/SolarSystem.h"
#include "systems/EarthBreakupSystem.h"

namespace {
    class SolarTrack : public TimelineTrack {
    public:
        SolarTrack(SolarSystem& solar, Scene& scene, MeteorShower& shower, const EarthBreakupSystem* breakup)
            : mSolar(solar),mScene(scene),mShower(shower),mBreakup(breakup) {}
        void update(float previous,float time) override {
            mSolar.update(mScene,time,time-previous,mBreakup && mBreakup->active());
            if(auto* earth=mScene.findObject("Earth")) mShower.followTarget(earth->transform.position);
        }
        void reset(float) override {
            mSolar.reset(mScene);
            if(auto* earth=mScene.findObject("Earth")) mShower.followTarget(earth->transform.position);
        }
    private:
        SolarSystem& mSolar; Scene& mScene; MeteorShower& mShower; const EarthBreakupSystem* mBreakup;
    };
}


bool MainSequence::build(Timeline& timeline, CinematicCamera& camera, Scene& scene, MeteorShower& shower,
    SolarSystem* solar, const EarthBreakupSystem* breakup)
{
    auto* earth = scene.findObject("Earth");
    if (!earth || !scene.findObject("EarthClouds")) return false;
    MeteorShowerConfig config;
    config.spawnRate = 0.5f;
    // Far above every authored shot: meteors travel into the image, never spawn in it.
    config.origin = earth->transform.position + glm::vec3(0, 160, 0);
    config.spawnHalfExtents = {110, 10, 110};
    config.direction = {0, -1, 0};
    config.aimed = true;
    config.target = earth->transform.position;
    config.targetRadius = 8.5f;
    config.minSpeed = 12;
    config.maxSpeed = 18;
    config.minScale = 0.15f;
    config.maxScale = 0.35f;
    config.minLifetime = 30;
    config.maxLifetime = 34;
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
    camera.orbitTargetTrack().addKeyframe(90, earth->transform.position);
    camera.orbitTargetTrack().addKeyframe(96, SolarSystem::SunCenter,EasingType::EaseInOut);
    camera.orbitTargetTrack().addKeyframe(Duration, SolarSystem::SunCenter);
    auto easing = EasingType::EaseInOut;
    camera.orbitRadiusTrack().addKeyframe(0, 34, easing);
    camera.orbitRadiusTrack().addKeyframe(8, 36, easing);
    camera.orbitRadiusTrack().addKeyframe(24, 42, easing);
    camera.orbitRadiusTrack().addKeyframe(40, 50, easing);
    camera.orbitRadiusTrack().addKeyframe(52, 65, easing);
    camera.orbitRadiusTrack().addKeyframe(62, 110, easing);
    camera.orbitRadiusTrack().addKeyframe(78, 210, easing);
    camera.orbitRadiusTrack().addKeyframe(90, 240, easing);
    camera.orbitRadiusTrack().addKeyframe(96, 1800, easing);
    camera.orbitRadiusTrack().addKeyframe(100, 6500, easing);
    camera.orbitRadiusTrack().addKeyframe(105, 16000, easing);
    camera.orbitRadiusTrack().addKeyframe(Duration, 35000, easing);
    camera.orbitYawTrack().addKeyframe(0, 15, easing);
    camera.orbitYawTrack().addKeyframe(24, 40, easing);
    camera.orbitYawTrack().addKeyframe(52, 80, easing);
    camera.orbitYawTrack().addKeyframe(78, 105, easing);
    camera.orbitYawTrack().addKeyframe(90, 110, easing);
    camera.orbitYawTrack().addKeyframe(96, 150, easing);
    camera.orbitYawTrack().addKeyframe(Duration, 150, easing);
    camera.orbitPitchTrack().addKeyframe(0, 8, easing);
    camera.orbitPitchTrack().addKeyframe(52, 10, easing);
    camera.orbitPitchTrack().addKeyframe(90, 6, easing);
    camera.orbitPitchTrack().addKeyframe(96, 55, easing);
    camera.orbitPitchTrack().addKeyframe(Duration, 55, easing);
    camera.fovTrack().addKeyframe(0, 45, easing);
    camera.fovTrack().addKeyframe(40, 48, easing);
    camera.fovTrack().addKeyframe(62, 52, easing);
    camera.fovTrack().addKeyframe(96, 45, easing);
    camera.fovTrack().addKeyframe(Duration, 45, easing);

    timeline.setDuration(Duration);

    const auto initialEarth=earth->transform.position;
    if(solar) timeline.addTrack(std::make_unique<SolarTrack>(*solar,scene,shower,breakup));
    timeline.addTrack(std::make_unique<CameraTrack>(camera,[&scene,initialEarth](float time) {
        const auto* current=scene.findObject("Earth");
        return current ? (current->transform.position-initialEarth)*(1.0f-glm::smoothstep(90.0f,96.0f,time)) : glm::vec3(0);
    }));

    auto animateRotation = [&](const std::string& name, float degreesPerSecond)
    {
        auto track = std::make_unique<TransformTrack>([&scene, name]() -> Transform*
        {
            auto* object = scene.findObject(name);
            return object ? &object->transform : nullptr;
        });
        track->rotationTrack().addKeyframe(0, glm::vec3(0));
        track->rotationTrack().addKeyframe(Duration, glm::vec3(0, glm::radians(Duration * degreesPerSecond), 0));
        timeline.addTrack(std::move(track));
    };
    animateRotation("Earth", 2.0f);
    animateRotation("EarthClouds", 2.3f);

    auto events = std::make_unique<EventTrack>();
    events->addEvent(BombardmentStart, [&shower] { shower.start(); });
    events->addEvent(24, [&shower] { shower.setEmission(2.0f,0.18f,0.5f); });
    events->addEvent(40, [&shower] { shower.setEmission(5.0f,0.22f,0.75f); });
    events->addEvent(48, [&shower] { shower.setEmission(9.0f,0.25f,1.1f); });
    events->addEvent(BombardmentEnd, [&shower] { shower.stop(); });
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
