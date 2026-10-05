#pragma once

#include <vector>

#include "animation/Keyframe.h"

// Samples chronological keyframes with interpolation and easing.
template <typename T>
class AnimationTrack
{
public:
    void addKeyframe(float time, const T& value, EasingType easing = EasingType::Linear)
    {
        mKeyframes.push_back({time, value, easing});
    }

    T evaluate(float time) const
    {
        if (mKeyframes.empty())
        {
            return T{};
        }

        if (time <= mKeyframes.front().time)
        {
            return mKeyframes.front().value;
        }

        if (time >= mKeyframes.back().time)
        {
            return mKeyframes.back().value;
        }

        for (std::size_t i = 0; i + 1 < mKeyframes.size(); ++i)
        {
            const auto& a = mKeyframes[i];
            const auto& b = mKeyframes[i + 1];

            if (time >= a.time && time <= b.time)
            {
                const float progress = (time - a.time) / (b.time - a.time);
                const float eased = Easing::apply(progress, a.easing);
                return a.value + (b.value - a.value) * eased;
            }
        }

        return mKeyframes.back().value;
    }

    bool empty() const { return mKeyframes.empty(); }

private:
    std::vector<Keyframe<T>> mKeyframes;
};
