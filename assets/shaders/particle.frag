#version 330 core

in vec4 vColor;

out vec4 fragColor;

void main()
{
    vec2 offset = gl_PointCoord * 2.0 - 1.0;
    float falloff = clamp(1.0 - dot(offset, offset), 0.0, 1.0);
    fragColor = vec4(vColor.rgb * falloff * falloff * 3.0, vColor.a * falloff);
}
