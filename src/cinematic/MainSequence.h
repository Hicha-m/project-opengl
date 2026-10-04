#pragma once
#include "animation/Timeline.h"
#include "camera/CinematicCamera.h"
#include "scene/Scene.h"
#include "systems/MeteorShower.h"

namespace MainSequence
{
    // Configure an empty timeline; scene, camera and shower must outlive its bindings.
    bool build(Timeline& timeline, CinematicCamera& camera, Scene& scene, MeteorShower& shower);
    // Reset controlled state and leave playback stopped. Call timeline.play() to replay.
    void reset(Timeline& timeline, MeteorShower& shower, MeteorSystem& system);
}
