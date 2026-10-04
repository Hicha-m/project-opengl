#pragma once
#include "animation/TimelineTrack.h"
#include "camera/CinematicCamera.h"

// The camera must outlive its track. Configure keyframes on CinematicCamera.
class CameraTrack : public TimelineTrack
{
public:
    explicit CameraTrack(CinematicCamera& camera) : mCamera(camera) {}
    void update(float, float time) override { mCamera.update(time); }
    void reset(float time) override { mCamera.update(time); }
private:
    CinematicCamera& mCamera;
};
