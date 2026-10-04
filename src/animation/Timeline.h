#pragma once
#include <memory>
#include <vector>
#include "animation/TimelineTrack.h"

class Timeline
{
public:
    void play() { mPlaying = true; }
    void pause() { mPlaying = false; }
    void stop();
    void reset();
    void update(float deltaTime);
    float getTime() const { return mCurrentTime; }
    bool isPlaying() const { return mPlaying; }
    void setDuration(float duration);
    float getDuration() const { return mDuration; }
    void addTrack(std::unique_ptr<TimelineTrack> track);
private:
    float mCurrentTime = 0;
    float mDuration = 0; // zero means unlimited
    bool mPlaying = false;
    std::vector<std::unique_ptr<TimelineTrack>> mTracks;
};
