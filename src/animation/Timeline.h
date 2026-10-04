#pragma once

#include <vector>
#include <functional>

#include "AnimationTrack.h"

class Timeline
{
public:

    Timeline();


    void play();

    void pause();

    void stop();

    void reset();


    void update(
        float deltaTime
    );


    float getTime() const;

    bool isPlaying() const;


    void setDuration(
        float duration
    );


    float getDuration() const;

    void addCallback(std::function<void(float)> callback
    
);


private:

    float mCurrentTime;

    float mDuration;

    bool mPlaying;


    std::vector<
        std::function<void(float)>
    > mCallbacks;
};