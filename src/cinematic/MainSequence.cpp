#include "cinematic/MainSequence.h"
#include "animation/CameraTrack.h"
#include "animation/TransformTrack.h"
#include "animation/EventTrack.h"
#include "systems/SolarSystem.h"
#include "systems/EarthBreakupSystem.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

namespace {
    const glm::vec3 escapeDirection=glm::normalize(glm::vec3(0.287f,0.819f,-0.497f));
    bool isShuttle(const SceneObject& object) { return object.name=="Shuttle" || object.name.rfind("Shuttle/",0)==0; }
    class EscapeTrack : public TimelineTrack {
    public:
        EscapeTrack(Scene& scene,CinematicCamera& camera,bool movingEarth)
            : mScene(scene),mCamera(camera) {
            mOrigin=movingEarth?SolarSystem::planetPosition(SolarSystem::Planets[2],30):scene.findObject("Earth")->transform.position;
            const float times[]={0,30,36,44,52,62,78,90,96,100,105,MainSequence::Duration};
            const float distances[]={8,8,14,26,55,110,240,500,1800,6500,16000,55000};
            for(unsigned i=0;i<12;++i) {
                const auto easing=i<3?EasingType::EaseInOut:EasingType::Linear;
                mDistance.addKeyframe(times[i],distances[i],easing);
            }
        }
        void update(float,float time) override { apply(time); }
        void reset(float time) override { apply(time); }
    private:
        void apply(float time) {
            const glm::vec3 position=mOrigin+escapeDirection*mDistance.evaluate(time);
            const glm::vec3 rotation(std::atan2(escapeDirection.y,-escapeDirection.z),-std::asin(escapeDirection.x),0);
            for(auto& object:mScene.objects) if(isShuttle(object)) {
                object.transform.position=position; object.transform.rotation=rotation;
                object.transform.scale=glm::vec3(6); object.visible=time>=30;
            }
            // Keep the departing ship in the upper half of the Earth shot.
            const auto* earth=mScene.findObject("Earth");
            const float departure=0.35f*glm::smoothstep(30.0f,42.0f,time);
            if(earth && time>=30) mCamera.setPose(mCamera.getPosition(),
                glm::mix(mCamera.target(),position,departure));
            const float follow=glm::smoothstep(46.0f,60.0f,time);
            if(follow>0) {
                const glm::vec3 up(0,0.519f,0.855f), right(0.958f,-0.246f,0.149f);
                const float side=1-glm::smoothstep(78.0f,94.0f,time);
                const auto cameraOffset=escapeDirection*12.0f+(up*3.0f+right*4.0f)*side;
                const float orbitAngle=glm::radians(-18.0f)*glm::smoothstep(60.0f,78.0f,time);
                const auto rotatedOffset=glm::vec3(glm::rotate(glm::mat4(1.0f),orbitAngle,up)*glm::vec4(cameraOffset,1.0f));
                const auto eye=position+rotatedOffset;
                auto framedEye=glm::mix(mCamera.getPosition(),eye,follow);
                const auto target=glm::mix(mCamera.target(),position,follow);
                // During the handoff, keep both the planet and craft inside the
                // vertical field of view instead of passing between them.
                if(earth && time<60) {
                    const auto outward=glm::normalize(framedEye-target);
                    float distance=glm::distance(framedEye,target);
                    const float halfFov=glm::radians(mCamera.getFOV()*0.5f);
                    auto fit=[&](const glm::vec3& center,float radius) {
                        const auto offset=center-target;
                        const float along=glm::dot(offset,outward);
                        const float transverse=glm::length(offset-outward*along);
                        distance=std::max(distance,along+transverse/std::tan(halfFov)+radius/std::sin(halfFov));
                    };
                    fit(earth->transform.position,10.0f*(1-glm::smoothstep(58.0f,60.0f,time)));
                    fit(position,3.2f);
                    const auto fittedEye=target+outward*distance;
                    const float fitWeight=glm::smoothstep(46.0f,52.0f,time);
                    framedEye=glm::mix(framedEye,fittedEye,fitWeight);
                }
                mCamera.setPose(framedEye,target);
            }
        }
        Scene& mScene; CinematicCamera& mCamera; glm::vec3 mOrigin; AnimationTrack<float> mDistance;
    };
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

    if(scene.findObject("Shuttle")) timeline.addTrack(std::make_unique<EscapeTrack>(scene,camera,solar!=nullptr));

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

void MainSequence::continueEscape(Scene& scene,CinematicCamera& camera,float deltaTime)
{
    if(!scene.findObject("Shuttle") || !std::isfinite(deltaTime) || deltaTime<=0) return;
    const auto displacement=escapeDirection*(4000.0f*deltaTime);
    for(auto& object:scene.objects) if(isShuttle(object)) object.transform.position+=displacement;
    camera.setPose(camera.getPosition()+displacement,camera.target()+displacement);
}
