#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec4 aColor;
layout(location = 2) in float aSize;

uniform mat4 uViewProjection;
uniform float uViewportHeight;

out vec4 vColor;

void main()
{
    vec4 clipPosition = uViewProjection * vec4(aPosition, 1.0);
    gl_Position = clipPosition;
    gl_PointSize = clamp(aSize * uViewportHeight / max(clipPosition.w, 0.05), 1.0, 64.0);
    vColor = aColor;
}
