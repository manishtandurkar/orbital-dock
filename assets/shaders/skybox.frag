#version 330 core

in vec3 vDirection;

uniform samplerCube uSky;
uniform float uBrightness;
uniform vec3 uSunDirection;
uniform vec3 uSunColor;

out vec4 fragColor;

void main()
{
    vec3 direction = normalize(vDirection);
    vec3 sky = pow(texture(uSky, direction).rgb, vec3(2.2)) * uBrightness;
    float alignment = max(dot(direction, uSunDirection), 0.0);
    float disc = smoothstep(0.99985, 0.99992, alignment);
    float corona = pow(alignment, 900.0) * 0.8 + pow(alignment, 60.0) * 0.12;
    vec3 sun = uSunColor * (disc * 40.0 + corona * 3.0);
    fragColor = vec4(sky + sun, 1.0);
}
