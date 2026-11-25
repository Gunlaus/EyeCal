#pragma once

#include <array>

namespace calib::ui::theme {

using Colour = std::array<float, 4>;
inline constexpr Colour canvas{0.052f, 0.060f, 0.070f, 1.0f};
inline constexpr Colour panel{0.085f, 0.098f, 0.115f, 1.0f};
inline constexpr Colour button{0.112f, 0.128f, 0.148f, 1.0f};
inline constexpr Colour accent{0.49f, 0.68f, 0.83f, 1.0f};
inline constexpr Colour focus{0.165f, 0.225f, 0.278f, 1.0f};
inline constexpr Colour selected{0.115f, 0.175f, 0.227f, 1.0f};
inline constexpr Colour text{0.93f, 0.945f, 0.96f, 1.0f};
inline constexpr Colour muted{0.73f, 0.76f, 0.80f, 1.0f};
inline constexpr int spacing = 12;
inline constexpr int baseline = 25;
inline constexpr float titleScale = 1.25f;
inline constexpr float radius = 9.0f;
inline constexpr auto transitionMs = 180;

inline Colour shade(Colour colour, float change)
{
    for (int i = 0; i < 3; ++i) {
        colour[i] += change;
    }
    return colour;
}

} // namespace calib::ui::theme
