#pragma once
#include "../animation/Timeline.h"
#include "../animation/CinematicCamera.h"
#include "../scene/Scene.h"

namespace MainSequence
{
    // Configure an empty timeline; replace camera keyframes. Scene and camera must outlive it.
    bool build(Timeline& timeline, CinematicCamera& camera, Scene& scene);
}
