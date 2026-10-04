#include "Timeline.h"

#include <algorithm>


Timeline::Timeline()
    : mCurrentTime(0.0f),
      mDuration(0.0f),
      mPlaying(false)
{
}


void Timeline::play()
{
    mPlaying = true;
}


void Timeline::pause()
{
    mPlaying = false;
}


void Timeline::stop()
{
    mPlaying = false;
    mCurrentTime = 0.0f;
}


void Timeline::reset()
{
    mCurrentTime = 0.0f;
}


void Timeline::update(
    float deltaTime
)
{
    if (!mPlaying)
        return;


    mCurrentTime += deltaTime;


    if (mDuration > 0.0f &&
        mCurrentTime > mDuration)
    {
        mCurrentTime =
            mDuration;

        mPlaying = false;
    }


    for (auto& callback : mCallbacks)
    {
        callback(
            mCurrentTime
        );
    }
}


float Timeline::getTime() const
{
    return mCurrentTime;
}


bool Timeline::isPlaying() const
{
    return mPlaying;
}


void Timeline::setDuration(
    float duration
)
{
    mDuration =
        std::max(
            0.0f,
            duration
        );
}


float Timeline::getDuration() const
{
    return mDuration;
}

void Timeline::addCallback(
    std::function<void(float)> callback
)
{
    mCallbacks.push_back(
        callback
    );
}