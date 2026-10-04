#include "Easing.h"

#include <algorithm>

float Easing::apply(
    float t,
    EasingType type
)
{
    t = std::clamp(
        t,
        0.0f,
        1.0f
    );

    switch (type)
    {
        case EasingType::Linear:
            return t;

        case EasingType::EaseIn:
            return t * t;

        case EasingType::EaseOut:
            return 1.0f - (1.0f - t) * (1.0f - t);

        case EasingType::EaseInOut:
        {
            if (t < 0.5f)
                return 2.0f * t * t;

            return 1.0f -
                   2.0f * (1.0f - t) *
                   (1.0f - t);
        }
    }

    return t;
}