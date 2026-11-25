#include "patterns/quantize.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace calib::patterns {
namespace {

constexpr std::array<std::uint32_t, 64> blueRanks{
#include "blue_noise_ranks.inc"
};

std::uint32_t bayerRank(Pixel pixel)
{
    std::uint32_t rank = 0;
    for (unsigned bit = 0; bit < 3; ++bit) {
        const auto x = (pixel.x >> bit) & 1u;
        const auto y = (pixel.y >> bit) & 1u;
        rank = (rank << 2) | ((x ^ y) << 1) | y;
    }
    return rank;
}

std::uint32_t hash(std::uint32_t value)
{
    value ^= value >> 16;
    value *= 0x7feb352du;
    value ^= value >> 15;
    value *= 0x846ca68bu;
    return value ^ (value >> 16);
}

float unitSample(std::uint32_t word)
{
    return static_cast<float>(word >> 8) * (1.0f / 16777216.0f);
}

} // namespace

std::uint32_t blueNoiseRank(Pixel pixel)
{
    const auto index = (pixel.y & 15u) * 16u + (pixel.x & 15u);
    return (blueRanks[index >> 2] >> (8 * (index & 3u))) & 255u;
}

float ditherOffset(Dither mode, Pixel pixel, std::uint32_t seed)
{
    switch (mode) {
    case Dither::off:
        return 0.0f;
    case Dither::ordered:
        return (static_cast<float>(bayerRank(pixel)) + 0.5f) * (1.0f / 64.0f) - 0.5f;
    case Dither::blueNoise:
        return (static_cast<float>(blueNoiseRank(pixel)) + 0.5f) * (1.0f / 256.0f) - 0.5f;
    case Dither::tpdf:
        const auto key = hash(pixel.x ^ seed ^ 0xa511e9b3u) ^ hash(pixel.y ^ 0x63d83595u);
        return unitSample(hash(key)) - unitSample(hash(key ^ 0x9e3779b9u));
    }
    return 0.0f;
}

std::uint32_t quantizeCode(float value, OutputDepth depth, Dither mode, Pixel pixel,
                           std::uint32_t seed)
{
    const auto maxCode = (1u << static_cast<std::uint32_t>(depth)) - 1u;
    if (std::isnan(value) || value <= 0.0f) {
        return 0;
    }
    if (value >= 1.0f) {
        return maxCode;
    }
    const float scaled = value * static_cast<float>(maxCode);
    const float shifted = scaled + ditherOffset(mode, pixel, seed);
    return static_cast<std::uint32_t>(
        std::clamp(std::floor(shifted + 0.5f), 0.0f, static_cast<float>(maxCode)));
}

float rampSample(std::uint32_t column, std::uint32_t width)
{
    if (width <= 1) {
        return 0.0f;
    }
    return static_cast<float>(std::min(column, width - 1)) / static_cast<float>(width - 1);
}

} // namespace calib::patterns
