#version 450
#extension GL_GOOGLE_include_directive : require
#include "pq.glsl"
layout(constant_id = 0) const uint hdrUi = 0u;
layout(push_constant) uniform Overlay {
    ivec4 rect;
    ivec2 viewport;
    float cellScale;
    int solid;
    uvec4 glyph[4];
    vec4 ink;
    vec4 shape;
} overlay;

layout(location = 0) out vec4 color;

float distanceSample(ivec2 cell)
{
    if (any(lessThan(cell, ivec2(0))) || any(greaterThanEqual(cell, ivec2(16, 16)))) {
        return 0.0;
    }
    int index = cell.y * 16 + cell.x;
    int word = index >> 4;
    uint packed = overlay.glyph[word >> 2][word & 3];
    return float((packed >> (2 * (index & 15))) & 3u);
}

float segment(vec2 p, vec2 a, vec2 b)
{
    vec2 ab = b - a;
    return length(p - a - ab * clamp(dot(p - a, ab) / dot(ab, ab), 0.0, 1.0));
}

void main()
{
    color = overlay.ink;
    if (overlay.solid == 1) {
        vec2 halfSize = 0.5 * vec2(overlay.rect.zw);
        vec2 local = gl_FragCoord.xy - vec2(overlay.rect.xy) - halfSize;
        float radius = min(overlay.shape.x, min(halfSize.x, halfSize.y));
        vec2 q = abs(local) - halfSize + radius;
        float d = length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - radius;
        color.a *= 1.0 - smoothstep(-0.8, 0.2, d);
        color.rgb += overlay.shape.y * smoothstep(-1.8, -0.45, d);
    } else if (overlay.solid > 1) {
        vec2 p = (gl_FragCoord.xy - vec2(overlay.rect.xy)) / vec2(overlay.rect.zw);
        float d;
        if (overlay.solid == 4) {
            d = min(min(abs(p.y - 0.28), abs(p.y - 0.5)), abs(p.y - 0.72));
            d = max(d, abs(p.x - 0.5) - 0.29);
        } else {
            if (overlay.solid == 3) p.x = 1.0 - p.x;
            d = min(segment(p, vec2(0.25, 0.5), vec2(0.75, 0.5)),
                    min(segment(p, vec2(0.25, 0.5), vec2(0.53, 0.23)),
                        segment(p, vec2(0.25, 0.5), vec2(0.53, 0.77))));
        }
        color.a *= 1.0 - smoothstep(0.035, 0.091, d);
    } else {
        vec2 pixel = (gl_FragCoord.xy - vec2(overlay.rect.xy)) / float(overlay.cellScale) - 0.5;
        ivec2 cell = ivec2(floor(pixel));
        vec2 fraction = fract(pixel);
        float upper = mix(distanceSample(cell), distanceSample(cell + ivec2(1, 0)), fraction.x);
        float lower = mix(distanceSample(cell + ivec2(0, 1)),
                          distanceSample(cell + ivec2(1, 1)), fraction.x);
        color.a *= smoothstep(0.9, 2.1, mix(upper, lower, fraction.y));
    }
    if (hdrUi != 0u) {
        vec3 srgb = clamp(color.rgb, vec3(0.0), vec3(1.0));
        vec3 rgb = mix(srgb / 12.92, pow((srgb + 0.055) / 1.055, vec3(2.4)),
                       greaterThan(srgb, vec3(0.04045)));
        mat3 to2020 = mat3(0.627404, 0.069097, 0.016391,
                           0.329283, 0.919540, 0.088013,
                           0.043313, 0.011362, 0.895595);
        vec3 nits = 100.0 * (to2020 * rgb);
        color.rgb = vec3(pqEncode(nits.r), pqEncode(nits.g), pqEncode(nits.b));
    }
}
