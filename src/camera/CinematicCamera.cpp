#include "camera/CinematicCamera.h"

#include <cmath>


CinematicCamera::CinematicCamera()
{
    mPosition =
        glm::vec3(0.0f, 0.0f, 10.0f);

    mTargetPos =
        glm::vec3(0.0f);

    mFOV = 45.0f;

    updateVectors();
}


// ============================================================
// UPDATE
// ============================================================

void CinematicCamera::update(float time)
{
    // --------------------------------------------------------
    // Orbit mode
    // --------------------------------------------------------

    if (mOrbitEnabled)
    {
        updateOrbit(time);
    }

    // --------------------------------------------------------
    // Free cinematic movement
    // --------------------------------------------------------

    else
    {
        if (!mPositionTrack.empty())
        {
            mPosition =
                mPositionTrack.evaluate(time);
        }


        if (!mTargetTrack.empty())
        {
            mTargetPos =
                mTargetTrack.evaluate(time);
        }
    }


    // --------------------------------------------------------
    // FOV
    // --------------------------------------------------------

    if (!mFovTrack.empty())
    {
        mFOV =
            mFovTrack.evaluate(time);
    }


    updateVectors();
}


// ============================================================
// TRACK ACCESS
// ============================================================

AnimationTrack<glm::vec3>&
CinematicCamera::positionTrack()
{
    return mPositionTrack;
}


AnimationTrack<glm::vec3>&
CinematicCamera::targetTrack()
{
    return mTargetTrack;
}


AnimationTrack<float>&
CinematicCamera::fovTrack()
{
    return mFovTrack;
}


// ============================================================
// ORBIT
// ============================================================

void CinematicCamera::enableOrbit(bool enabled)
{
    mOrbitEnabled = enabled;
}


AnimationTrack<float>&
CinematicCamera::orbitRadiusTrack()
{
    return mOrbitRadiusTrack;
}


AnimationTrack<float>&
CinematicCamera::orbitYawTrack()
{
    return mOrbitYawTrack;
}


AnimationTrack<float>&
CinematicCamera::orbitPitchTrack()
{
    return mOrbitPitchTrack;
}


AnimationTrack<glm::vec3>&
CinematicCamera::orbitTargetTrack()
{
    return mOrbitTargetTrack;
}


// ============================================================
// UPDATE ORBIT
// ============================================================

void CinematicCamera::updateOrbit(float time)
{
    float radius = 10.0f;
    float yaw = 0.0f;
    float pitch = 0.0f;

    glm::vec3 target(0.0f);


    if (!mOrbitRadiusTrack.empty())
    {
        radius =
            mOrbitRadiusTrack.evaluate(time);
    }


    if (!mOrbitYawTrack.empty())
    {
        yaw =
            mOrbitYawTrack.evaluate(time);
    }


    if (!mOrbitPitchTrack.empty())
    {
        pitch =
            mOrbitPitchTrack.evaluate(time);
    }


    if (!mOrbitTargetTrack.empty())
    {
        target =
            mOrbitTargetTrack.evaluate(time);
    }


    mTargetPos = target;


    // Les tracks utilisent des degrés.
    float yawRad =
        glm::radians(yaw);

    float pitchRad =
        glm::radians(pitch);


    mPosition.x =
        target.x +
        radius *
        cosf(pitchRad) *
        sinf(yawRad);


    mPosition.y =
        target.y +
        radius *
        sinf(pitchRad);


    mPosition.z =
        target.z +
        radius *
        cosf(pitchRad) *
        cosf(yawRad);
}


// ============================================================
// CAMERA VECTORS
// ============================================================

void CinematicCamera::updateVectors()
{
    glm::vec3 direction =
        mTargetPos - mPosition;


    if (glm::length(direction) < 0.0001f)
        return;


    mLook =
        glm::normalize(direction);


    mRight =
        glm::normalize(
            glm::cross(
                mLook,
                WORLD_UP
            )
        );


    mUp =
        glm::normalize(
            glm::cross(
                mRight,
                mLook
            )
        );
}