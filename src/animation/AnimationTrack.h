#pragma once

#include <vector>

#include "animation/Keyframe.h"

template<typename T>
class AnimationTrack
{
public:

    void addKeyframe(
        float time,
        const T& value,
        EasingType easing =
            EasingType::Linear
    )
    {
        Keyframe<T> keyframe;

        keyframe.time = time;
        keyframe.value = value;
        keyframe.easing = easing;

        mKeyframes.push_back(
            keyframe
        );
    }


    T evaluate(float time) const
    {
        if (mKeyframes.empty())
            return T{};


        if (time <= mKeyframes.front().time)
            return mKeyframes.front().value;


        if (time >= mKeyframes.back().time)
            return mKeyframes.back().value;


        for (size_t i = 0;
             i + 1 < mKeyframes.size();
             ++i)
        {
            const Keyframe<T>& a =
                mKeyframes[i];

            const Keyframe<T>& b =
                mKeyframes[i + 1];


            if (time >= a.time &&
                time <= b.time)
            {
                float duration =
                    b.time - a.time;

                float t =
                    (time - a.time) /
                    duration;

                t =
                    Easing::apply(
                        t,
                        a.easing
                    );

                return interpolate(
                    a.value,
                    b.value,
                    t
                );
            }
        }

        return mKeyframes.back().value;
    }


    bool empty() const
    {
        return mKeyframes.empty();
    }


private:

    std::vector<Keyframe<T>>
        mKeyframes;


    static T interpolate(
        const T& a,
        const T& b,
        float t
    )
    {
        return a + (b - a) * t;
    }
};