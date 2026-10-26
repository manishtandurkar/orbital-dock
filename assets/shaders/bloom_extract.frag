#version 330 core

in vec2 vUv;

uniform sampler2D uScene;
uniform float uThreshold;

out vec4 fragColor;

void main()
{
    vec3 color = texture(uScene, vUv).rgb;
    float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
    float weight = smoothstep(uThreshold, uThreshold * 2.0, luminance);
    fragColor = vec4(color * weight, 1.0);
}
