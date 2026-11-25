#version 450
layout(push_constant) uniform Overlay {
    ivec4 rect;
    ivec2 viewport;
    int cellScale;
    int solid;
    uvec4 glyph[4];
    vec4 ink;
} overlay;

layout(location = 0) out vec4 color;

float distanceSample(ivec2 cell)
{
    if (any(lessThan(cell, ivec2(0))) || any(greaterThanEqual(cell, ivec2(12, 20)))) {
        return 0.0;
    }
    int index = cell.y * 12 + cell.x;
    int word = index >> 4;
    uint packed = overlay.glyph[word >> 2][word & 3];
    return float((packed >> (2 * (index & 15))) & 3u);
}

void main()
{
    color = overlay.ink;
    if (overlay.solid == 0) {
        vec2 pixel = (gl_FragCoord.xy - vec2(overlay.rect.xy)) / float(overlay.cellScale) - 0.5;
        ivec2 cell = ivec2(floor(pixel));
        vec2 fraction = fract(pixel);
        float upper = mix(distanceSample(cell), distanceSample(cell + ivec2(1, 0)), fraction.x);
        float lower = mix(distanceSample(cell + ivec2(0, 1)),
                          distanceSample(cell + ivec2(1, 1)), fraction.x);
        color.a *= smoothstep(0.9, 2.1, mix(upper, lower, fraction.y));
    }
}
