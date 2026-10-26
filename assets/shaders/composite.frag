#version 330 core

in vec2 vUv;

uniform sampler2D uScene;
uniform sampler2D uBloom;
uniform float uExposure;
uniform float uBloomStrength;
uniform float uVignetteStrength;

out vec4 fragColor;

vec3 acesFilmic(vec3 color)
{
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
}

void main()
{
    vec3 hdr = texture(uScene, vUv).rgb + texture(uBloom, vUv).rgb * uBloomStrength;
    vec3 mapped = acesFilmic(hdr * uExposure);
    vec2 centred = vUv - 0.5;
    float vignette = 1.0 - dot(centred, centred) * uVignetteStrength;
    mapped *= clamp(vignette, 0.0, 1.0);
    fragColor = vec4(pow(mapped, vec3(1.0 / 2.2)), 1.0);
}
