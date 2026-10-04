#include "Timeline.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

void Timeline::addTrack(std::unique_ptr<TimelineTrack> track)
{
    if (!track) throw std::invalid_argument("Null timeline track");
    track->reset(mCurrentTime);
    mTracks.push_back(std::move(track));
}

void Timeline::stop()
{
    mPlaying = false;
    reset();
}

void Timeline::reset()
{
    mCurrentTime = 0;
    for (auto& track : mTracks) track->reset(0);
}

void Timeline::setDuration(float duration)
{
    if (!std::isfinite(duration) || duration < 0)
        throw std::invalid_argument("Invalid timeline duration");
    mDuration = duration;
    if (duration > 0 && mCurrentTime > duration)
    {
        mCurrentTime = duration;
        mPlaying = false;
        for (auto& track : mTracks) track->reset(mCurrentTime);
    }
}

void Timeline::update(float deltaTime)
{
    if (!mPlaying || !std::isfinite(deltaTime) || deltaTime <= 0) return;
    const float previous = mCurrentTime;
    mCurrentTime += deltaTime;
    if (mDuration > 0 && mCurrentTime >= mDuration)
    {
        mCurrentTime = mDuration;
        mPlaying = false;
    }
    for (auto& track : mTracks) track->update(previous, mCurrentTime);
}
