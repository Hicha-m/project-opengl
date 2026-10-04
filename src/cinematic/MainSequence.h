#pragma once
#include "animation/Timeline.h"
#include "camera/CinematicCamera.h"
#include "scene/Scene.h"
#include "systems/MeteorShower.h"

class SolarSystem;
class EarthBreakupSystem;

namespace MainSequence
{
    constexpr float Duration = 130.0f;
    constexpr float BombardmentStart = 8.0f;
    constexpr float BombardmentEnd = 62.0f;
    // Configure an empty timeline; scene, camera and shower must outlive its bindings.
    bool build(Timeline& timeline, CinematicCamera& camera, Scene& scene, MeteorShower& shower,
        SolarSystem* solar = nullptr, const EarthBreakupSystem* breakup = nullptr);
    // Reset controlled state and leave playback stopped. Call timeline.play() to replay.
    void reset(Timeline& timeline, MeteorShower& shower, MeteorSystem& system);
}
