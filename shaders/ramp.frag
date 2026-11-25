#version 450
#extension GL_GOOGLE_include_directive : require
#include "quantize.glsl"

layout(push_constant) uniform Ramp {
    ivec4 rect;
    ivec2 viewport;
    uint bits;
    uint dither;
    uint coarse;
} ramp;

layout(location = 0) out vec4 color;

void main()
{
    uvec2 pixel = uvec2(gl_FragCoord.xy);
    uint column = pixel.x - uint(ramp.rect.x);
    uint width = uint(ramp.rect.z);
    float value = width > 1u ? float(column) / float(width - 1u) : 0.0;
    if (ramp.coarse != 0u) {
        value = float(column * 8u / width) / 7.0;
    }
    color = vec4(quantizeUnorm(vec3(value), ramp.bits, ramp.dither, pixel, 0u), 1.0);
}
