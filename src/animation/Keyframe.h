#pragma once

#include "Easing.h"

template<typename T>
struct Keyframe
{
    float time = 0.0f;

    T value{};

    EasingType easing =
        EasingType::Linear;
};