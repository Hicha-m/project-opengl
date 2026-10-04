#pragma once
#include "animation/TimelineTrack.h"
#include "camera/CinematicCamera.h"
#include <functional>

// The camera must outlive its track. Configure keyframes on CinematicCamera.
class CameraTrack : public TimelineTrack
{
public:
    explicit CameraTrack(CinematicCamera& camera, std::function<glm::vec3(float)> offset = {})
        : mCamera(camera), mOffset(std::move(offset)) {}
    void update(float, float time) override { sample(time); }
    void reset(float time) override { sample(time); }
private:
    void sample(float time) {
        mCamera.setOrbitOffset(mOffset ? mOffset(time) : glm::vec3(0));
        mCamera.update(time);
    }
    CinematicCamera& mCamera;
    std::function<glm::vec3(float)> mOffset;
};
