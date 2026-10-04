#pragma once

// update covers (previousTime, time]; reset samples without firing events.
class TimelineTrack
{
public:
    virtual ~TimelineTrack() = default;
    virtual void update(float previousTime, float time) = 0;
    virtual void reset(float time) = 0;
};
