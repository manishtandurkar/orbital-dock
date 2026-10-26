#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUv;

uniform mat4 uModel;
uniform mat3 uNormalMatrix;
uniform mat4 uViewProjection;
uniform mat4 uLightSpace;

out vec3 vWorldPosition;
out vec3 vNormal;
out vec2 vUv;
out vec4 vLightSpacePosition;

void main()
{
    vec4 worldPosition = uModel * vec4(aPosition, 1.0);
    vWorldPosition = worldPosition.xyz;
    vNormal = uNormalMatrix * aNormal;
    vUv = aUv;
    vLightSpacePosition = uLightSpace * worldPosition;
    gl_Position = uViewProjection * worldPosition;
}
