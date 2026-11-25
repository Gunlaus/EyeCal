#pragma once

#include <cstdint>

namespace calib::patterns {

enum class Dither : std::uint32_t { off, ordered, blueNoise, tpdf };
enum class OutputDepth : std::uint32_t { unorm8 = 8, unorm10 = 10 };

struct Pixel {
    std::uint32_t x;
    std::uint32_t y;
};

std::uint32_t blueNoiseRank(Pixel pixel);
float ditherOffset(Dither mode, Pixel pixel, std::uint32_t seed = 0);
std::uint32_t quantizeCode(float value, OutputDepth depth, Dither mode = Dither::off,
                           Pixel pixel = {}, std::uint32_t seed = 0);
float rampSample(std::uint32_t column, std::uint32_t width);

} // namespace calib::patterns
