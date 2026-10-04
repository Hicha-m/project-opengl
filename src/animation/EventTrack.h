#pragma once
#include <algorithm>
#include <cmath>
#include <functional>
#include <stdexcept>
#include <vector>
#include "TimelineTrack.h"

class EventTrack : public TimelineTrack
{
public:
    void addEvent(float time, std::function<void()> callback)
    {
        if (!std::isfinite(time) || time < 0 || !callback)
            throw std::invalid_argument("Invalid timeline event");
        mEvents.push_back({time, std::move(callback)});
        std::stable_sort(mEvents.begin(), mEvents.end(),
            [](const Event& a, const Event& b) { return a.time < b.time; });
    }
    void update(float previousTime, float time) override
    {
        // Copy pending callbacks: callbacks may safely register more events.
        std::vector<std::function<void()>> pending;
        for (const auto& event : mEvents)
            if ((event.time > previousTime || (mAtStart && event.time == 0)) && event.time <= time)
                pending.push_back(event.callback);
        mAtStart = false;
        for (auto& callback : pending) callback();
    }
    void reset(float time) override { mAtStart = time == 0; }
private:
    struct Event { float time; std::function<void()> callback; };
    std::vector<Event> mEvents;
    bool mAtStart = true;
};
