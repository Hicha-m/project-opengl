#pragma once
#include <functional>
#include "TimelineTrack.h"
#include "AnimationTrack.h"
#include "../scene/Transform.h"

class TransformTrack : public TimelineTrack
{
public:
    // Resolve every frame so Scene::objects vector growth cannot invalidate a binding.
    // The scene/target must outlive the track; nullptr means the target is absent.
    explicit TransformTrack(std::function<Transform*()> target) : mTarget(std::move(target)) {}
    explicit TransformTrack(Transform& target) : TransformTrack([&target] { return &target; }) {}
    AnimationTrack<glm::vec3>& positionTrack() { return mPosition; }
    AnimationTrack<glm::vec3>& rotationTrack() { return mRotation; } // radians
    AnimationTrack<glm::vec3>& scaleTrack() { return mScale; }
    void update(float, float time) override { sample(time); }
    void reset(float time) override { sample(time); }
private:
    void sample(float time)
    {
        auto* target = mTarget();
        if (!target) return;
        if (!mPosition.empty()) target->position = mPosition.evaluate(time);
        if (!mRotation.empty()) target->rotation = mRotation.evaluate(time);
        if (!mScale.empty()) target->scale = mScale.evaluate(time);
    }
    std::function<Transform*()> mTarget;
    AnimationTrack<glm::vec3> mPosition, mRotation, mScale;
};
