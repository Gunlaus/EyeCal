#version 450
#extension GL_GOOGLE_include_directive : require
#include "quantize.glsl"
#include "calibration.glsl"
#include "pq.glsl"
#include "colour_hdr.glsl"

layout(push_constant) uniform Ramp {
    ivec4 rect;
    ivec2 viewport;
    uint bits;
    uint dither;
    uint coarse;
    uint pattern;
    uint target;
    float phase;
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
    uint mode = ramp.dither;
    bool bad = ramp.coarse != 0u;
    if (ramp.pattern >= 11u) {
        vec3 rgb;
        switch (ramp.pattern) {
        case 11u: rgb = greyBalance(local, size, bad); break;
        case 12u: rgb = colourSteps(local, size, bad); break;
        case 13u: rgb = channelClip(local, size, bad); break;
        case 14u: rgb = uniformField(ramp.target); break;
        default: rgb = vec3(pqEncode(hdrLuminance(ramp.pattern, local, size, ramp.target))); break;
        }
        color = vec4(quantizeUnorm(rgb, ramp.bits, DITHER_OFF, pixel, 0u), 1.0);
        return;
    }
    switch (ramp.pattern) {
    case 1u:
        value = size.y > 1u ? float(local.y) / float(size.y - 1u) : 0.0;
        if (bad) {
            value = float(local.y * 8u / size.y) / 7.0;
            mode = DITHER_OFF;
        }
        break;
    case 3u: value = rangeSteps(local, size, bad); break;
    case 4u: value = blackSteps(local, size, bad); break;
    case 5u: value = whiteSteps(local, size, bad); break;
    case 6u: value = gammaPatches(local, size, bad, ramp.target); break;
    case 7u: value = sharpnessGrid(local, size, bad); break;
    case 8u:
        value = size.y > 1u ? float(local.y) / float(size.y - 1u) : 0.0;
        mode = local.x < ramp.target ? DITHER_OFF : ramp.dither;
        break;
    case 9u:
        mode = DITHER_OFF;
        break;
    case 10u: {
        float t = 0.5 * value + ramp.phase;
        float wave = 1.0 - abs(2.0 * fract(t) - 1.0);
        float linearValue = 0.2 * (wave + (local.y < size.y / 2u ? 0.0 : 1.0));
        value = linearValue <= 0.0031308 ? 12.92 * linearValue :
                1.055 * pow(linearValue, 1.0 / 2.4) - 0.055;
        mode = DITHER_OFF;
        break;
    }
    default:
        if (bad) value = float(column * 8u / width) / 7.0;
        break;
    }
    color = vec4(quantizeUnorm(vec3(value), ramp.bits, mode, pixel, 0u), 1.0);
}
