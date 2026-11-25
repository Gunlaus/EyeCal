#pragma once

#include <algorithm>
#include <cmath>

namespace calib::patterns {

inline float pqEncode(float nits)
{
    const float p = std::pow(std::clamp(nits / 10000.0f, 0.0f, 1.0f), 2610.0f / 16384.0f);
    return std::pow((3424.0f / 4096.0f + (2413.0f / 128.0f) * p) /
                    (1.0f + (2392.0f / 128.0f) * p), 2523.0f / 32.0f);
}

} // namespace calib::patterns
