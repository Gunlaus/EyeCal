vec3 greyBalance(uvec2 p, uvec2 size, bool bad)
{
    const uint steps[8] = uint[8](16u, 32u, 64u, 96u, 128u, 176u, 224u, 255u);
    float grey = code(steps[columnIndex(p.x, size.x, 8u)]);
    return bad ? clamp(vec3(grey) + vec3(0.08, 0.0, -0.04) * sin(grey * 3.14159),
                       0.0, 1.0) : vec3(grey);
}

vec3 colourSteps(uvec2 p, uvec2 size, bool bad)
{
    const vec3 hues[6] = vec3[6](vec3(1,0,0), vec3(0,1,0), vec3(0,0,1),
                                vec3(0,1,1), vec3(1,0,1), vec3(1,1,0));
    const uint strengths[8] = uint[8](0u, 64u, 128u, 166u, 191u, 217u, 242u, 255u);
    uint col = columnIndex(p.x, size.x, 8u);
    float amount = bad && col >= 5u ? 1.0 : code(strengths[col]);
    return mix(vec3(code(128u)), hues[columnIndex(p.y, size.y, 6u)], amount);
}

vec3 channelClip(uvec2 p, uvec2 size, bool bad)
{
    const vec3 hues[3] = vec3[3](vec3(1,0,0), vec3(0,1,0), vec3(0,0,1));
    const uint steps[7] = uint[7](224u, 232u, 240u, 246u, 250u, 253u, 255u);
    uint col = columnIndex(p.x, size.x, 7u);
    return hues[columnIndex(p.y, size.y, 3u)] * code(bad && col >= 3u ? 255u : steps[col]);
}

vec3 uniformField(uint target)
{
    const vec3 fields[8] = vec3[8](vec3(13), vec3(51), vec3(128), vec3(255),
        vec3(255,0,0), vec3(0,255,0), vec3(0,0,255), vec3(0));
    return fields[min(target, 7u)] / 255.0;
}

float hdrLuminance(uint pattern, uvec2 p, uvec2 size, uint target)
{
    if (pattern == 18u) {
        const float areas[3] = float[3](0.02, 0.10, 0.25);
        vec2 position = (vec2(p) + 0.5) / vec2(size);
        return all(lessThan(abs(position - 0.5), vec2(0.5 * sqrt(areas[min(target, 2u)]))))
            ? 600.0 : 0.0;
    }
    if (pattern == 15u) {
        const float levels[8] = float[8](100, 200, 400, 600, 1000, 1500, 2000, 4000);
        if (p.y < size.y * 2u / 5u || p.y >= size.y * 3u / 5u) return 0.0;
        return levels[columnIndex(p.x, size.x, 8u)];
    }
    if (pattern == 16u) {
        const float levels[8] = float[8](0, .005, .01, .02, .05, .1, .5, 1);
        if (p.y < size.y / 4u || p.y >= size.y * 3u / 4u) return 0.0;
        return levels[columnIndex(p.x, size.x, 8u)];
    }
    const float levels[4] = float[4](80, 100, 203, 300);
    if (p.y < size.y / 3u || p.y >= size.y * 2u / 3u) return 0.0;
    return levels[columnIndex(p.x, size.x, 4u)];
}
