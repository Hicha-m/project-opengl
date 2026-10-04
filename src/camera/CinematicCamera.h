#pragma once

#include "camera/Camera.h"
#include "animation/AnimationTrack.h"

class CinematicCamera : public Camera
{
public:

    CinematicCamera();

    void update(float time);
    void setPose(const glm::vec3& eye, const glm::vec3& target) { mPosition=eye; mTargetPos=target; updateVectors(); }
    const glm::vec3& target() const { return mTargetPos; }
    void setOrbitOffset(const glm::vec3& offset) { mOrbitOffset=offset; }


    // ------------------------------------------------
    // Tracks
    // ------------------------------------------------

    AnimationTrack<glm::vec3>& positionTrack();

    AnimationTrack<glm::vec3>& targetTrack();

    AnimationTrack<float>& fovTrack();


    // ------------------------------------------------
    // Orbit cinématique
    // ------------------------------------------------

    void enableOrbit(bool enabled);

    AnimationTrack<float>& orbitRadiusTrack();

    AnimationTrack<float>& orbitYawTrack();

    AnimationTrack<float>& orbitPitchTrack();

    AnimationTrack<glm::vec3>& orbitTargetTrack();


private:

    AnimationTrack<glm::vec3>
        mPositionTrack;

    AnimationTrack<glm::vec3>
        mTargetTrack;

    AnimationTrack<float>
        mFovTrack;


    // ------------------------------------------------
    // Orbit
    // ------------------------------------------------

    bool mOrbitEnabled = false;
    glm::vec3 mOrbitOffset{0};

    AnimationTrack<float>
        mOrbitRadiusTrack;

    AnimationTrack<float>
        mOrbitYawTrack;

    AnimationTrack<float>
        mOrbitPitchTrack;

    AnimationTrack<glm::vec3>
        mOrbitTargetTrack;


    void updateOrbit(float time);

    void updateVectors();
};