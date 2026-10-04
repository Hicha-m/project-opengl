#pragma once

enum class EasingType
{
    Linear,
    EaseIn,
    EaseOut,
    EaseInOut
};

namespace Easing
{
    float apply(
        float t,
        EasingType type
    );
}