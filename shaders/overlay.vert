#version 450
layout(push_constant) uniform Overlay {
    ivec4 rect;
    ivec2 viewport;
    int cellScale;
    int solid;
    uvec4 glyph[4];
    vec4 ink;
} overlay;

void main()
{
    const ivec2 corners[6] = ivec2[6](ivec2(0, 0), ivec2(1, 0), ivec2(0, 1),
                                      ivec2(0, 1), ivec2(1, 0), ivec2(1, 1));
    vec2 pixel = vec2(overlay.rect.xy + corners[gl_VertexIndex] * overlay.rect.zw);
    gl_Position = vec4(2.0 * pixel / vec2(overlay.viewport) - 1.0, 0.0, 1.0);
}
