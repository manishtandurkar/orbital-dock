#version 330 core

in vec3 vDirection;

uniform samplerCube uSky;
uniform float uBrightness;

out vec4 fragColor;

void main()
{
    fragColor = vec4(texture(uSky, normalize(vDirection)).rgb * uBrightness, 1.0);
}
