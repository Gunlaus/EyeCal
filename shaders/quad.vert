#version 450
layout(push_constant) uniform Quad {
    ivec4 rect;
    ivec2 viewport;
} quad;

void main()
{
    const ivec2 corners[6] = ivec2[6](ivec2(0, 0), ivec2(1, 0), ivec2(0, 1),
                                      ivec2(0, 1), ivec2(1, 0), ivec2(1, 1));
    vec2 pixel = vec2(quad.rect.xy + corners[gl_VertexIndex] * quad.rect.zw);
    gl_Position = vec4(2.0 * pixel / vec2(quad.viewport) - 1.0, 0.0, 1.0);
}
