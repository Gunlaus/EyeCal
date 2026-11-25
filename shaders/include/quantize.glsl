#ifndef EYECAL_QUANTIZE_GLSL
#define EYECAL_QUANTIZE_GLSL

// Keep these values and the math aligned with src/patterns/quantize.cpp.
const uint DITHER_OFF = 0u;
const uint DITHER_ORDERED = 1u;
const uint DITHER_BLUE_NOISE = 2u;
const uint DITHER_TPDF = 3u;

// Tiled 16 × 16 rank texture, four 8-bit texels packed per word. Exact integer lookup.
const uint blueRanks[64] = uint[64](
#include "blue_noise_ranks.inc"
);

uint bayerRank(uvec2 pixel)
{
    uint rank = 0u;
    for (uint bit = 0u; bit < 3u; ++bit) {
        uint x = (pixel.x >> bit) & 1u;
        uint y = (pixel.y >> bit) & 1u;
        rank = (rank << 2u) | ((x ^ y) << 1u) | y;
    }
    return rank;
}

uint blueNoiseRank(uvec2 pixel)
{
    uint index = (pixel.y & 15u) * 16u + (pixel.x & 15u);
    return (blueRanks[index >> 2u] >> (8u * (index & 3u))) & 255u;
}

uint ditherHash(uint value)
{
    value ^= value >> 16u;
    value *= 0x7feb352du;
    value ^= value >> 15u;
    value *= 0x846ca68bu;
    return value ^ (value >> 16u);
}

float unitSample(uint word)
{
    return float(word >> 8u) * (1.0 / 16777216.0);
}

float ditherOffset(uint mode, uvec2 pixel, uint seed)
{
    switch (mode) {
    case DITHER_ORDERED:
        return (float(bayerRank(pixel)) + 0.5) * (1.0 / 64.0) - 0.5;
    case DITHER_BLUE_NOISE:
        return (float(blueNoiseRank(pixel)) + 0.5) * (1.0 / 256.0) - 0.5;
    case DITHER_TPDF:
        uint key = ditherHash(pixel.x ^ seed ^ 0xa511e9b3u) ^ ditherHash(pixel.y ^ 0x63d83595u);
        return unitSample(ditherHash(key)) - unitSample(ditherHash(key ^ 0x9e3779b9u));
    default:
        return 0.0;
    }
}

// Accept display-encoded values. Each pattern applies its own transfer function before this call.
float quantizeUnorm(float value, uint bits, uint mode, uvec2 pixel, uint seed)
{
    float maxCode = float((1u << bits) - 1u);
    if (isnan(value) || value <= 0.0) {
        return 0.0;
    }
    if (value >= 1.0) {
        return 1.0;
    }
    precise float scaled = value * maxCode;
    precise float shifted = scaled + ditherOffset(mode, pixel, seed);
    return clamp(floor(shifted + 0.5), 0.0, maxCode) / maxCode;
}

vec3 quantizeUnorm(vec3 value, uint bits, uint mode, uvec2 pixel, uint seed)
{
    return vec3(quantizeUnorm(value.r, bits, mode, pixel, seed),
                quantizeUnorm(value.g, bits, mode, pixel, seed),
                quantizeUnorm(value.b, bits, mode, pixel, seed));
}

#endif
