#version 450
#extension GL_GOOGLE_include_directive : require
#include "quantize.glsl"
#include "calibration.glsl"

layout(push_constant) uniform Ramp {
    ivec4 rect;
    ivec2 viewport;
    uint bits;
    uint dither;
    uint coarse;
    uint pattern;
    uint target;
} ramp;

layout(location = 0) out vec4 color;

void main()
{
    uvec2 pixel = uvec2(gl_FragCoord.xy);
    uint column = pixel.x - uint(ramp.rect.x);
    uint width = uint(ramp.rect.z);
    float value = width > 1u ? float(column) / float(width - 1u) : 0.0;
    uvec2 local = pixel - uvec2(ramp.rect.xy);
    uvec2 size = uvec2(ramp.rect.zw);
    bool bad = ramp.coarse != 0u;
    switch (ramp.pattern) {
    case 2u: value = setupSwatches(local, size, bad); break;
    case 3u: value = rangeSteps(local, size, bad); break;
    case 4u: value = blackSteps(local, size, bad); break;
    case 5u: value = whiteSteps(local, size, bad); break;
    case 6u: value = gammaPatches(local, size, bad, ramp.target); break;
    case 7u: value = sharpnessGrid(local, size, bad); break;
    default:
        if (bad) value = float(column * 8u / width) / 7.0;
        break;
    }
    color = vec4(quantizeUnorm(vec3(value), ramp.bits, ramp.dither, pixel, 0u), 1.0);
}
