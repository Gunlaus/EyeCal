float code(uint value)
{
    return float(value) / 255.0;
}

uint columnIndex(uint x, uint width, uint count)
{
    return min((x * count) / width, count - 1u);
}

float rangeSteps(uvec2 p, uvec2 size, bool bad)
{
    uint column = columnIndex(p.x, size.x, 9u);
    if (p.y < size.y / 2u) return code(bad ? 0u : column);
    return code(bad ? 255u : 247u + column);
}

float blackSteps(uvec2 p, uvec2 size, bool bad)
{
    const uint levels[10] = uint[10](0u, 1u, 2u, 3u, 4u, 6u, 8u, 12u, 16u, 24u);
    if (p.y < size.y / 4u || p.y >= 3u * size.y / 4u) return 0.0;
    uint value = levels[columnIndex(p.x, size.x, 10u)];
    return code(bad && value <= 8u ? 0u : value);
}

float whiteSteps(uvec2 p, uvec2 size, bool bad)
{
    const uint levels[9] = uint[9](235u, 240u, 245u, 247u, 249u, 251u, 253u, 254u, 255u);
    uint value = levels[columnIndex(p.x, size.x, 9u)];
    return code(bad && value >= 247u ? 255u : value);
}

float gammaPatches(uvec2 p, uvec2 size, bool bad, uint target)
{
    uint patchNumber = columnIndex(p.x, size.x, 3u) + 1u;
    if (p.y < size.y / 2u) return ((p.y & 1u) * 2u + (p.x & 1u)) < patchNumber ? 1.0 : 0.0;
    float coverage = float(patchNumber) * 0.25;
    float grey = target == 0u ? 1.055 * pow(coverage, 1.0 / 2.4) - 0.055 :
                 pow(coverage, target == 1u ? 1.0 / 2.2 : 1.0 / 2.4);
    return bad ? min(1.0, grey + 0.14) : grey;
}

float sharpnessGrid(uvec2 p, uvec2 size, bool bad)
{
    uvec2 edge = min(p, size - 1u - p);
    if (edge.x == 0u || edge.y == 0u) return code(224u);
    if ((edge.x < 18u && edge.y == 4u) || (edge.y < 18u && edge.x == 4u)) return 1.0;
    if (p.x < size.x / 2u) {
        bool line = p.y < size.y / 2u ? (p.x & 1u) == 0u : (p.y & 1u) == 0u;
        return code(line ? 192u : 64u);
    }
    uint boundary = size.x * 3u / 4u;
    if (bad && p.x + 2u >= boundary && p.x < boundary) return code(16u);
    if (bad && p.x >= boundary && p.x < boundary + 2u) return code(240u);
    return code(p.x < boundary ? 64u : 192u);
}
