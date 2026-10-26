#version 330 core

in vec2 vUv;

uniform sampler2D uImage;
uniform vec2 uDirection;

out vec4 fragColor;

void main()
{
    vec2 texel = uDirection / vec2(textureSize(uImage, 0));
    float weights[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);
    vec3 result = texture(uImage, vUv).rgb * weights[0];
    for (int i = 1; i < 5; ++i) {
        float offset = float(i) * 1.5;
        result += texture(uImage, vUv + texel * offset).rgb * weights[i];
        result += texture(uImage, vUv - texel * offset).rgb * weights[i];
    }
    fragColor = vec4(result, 1.0);
}
