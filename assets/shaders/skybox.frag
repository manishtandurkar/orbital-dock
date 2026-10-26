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
    vec3 sky = texture(uSky, direction).rgb * uBrightness;
    float alignment = max(dot(direction, uSunDirection), 0.0);
    float disc = smoothstep(0.99985, 0.99992, alignment);
    float corona = pow(alignment, 900.0) * 0.8 + pow(alignment, 60.0) * 0.12;
    vec3 sun = uSunColor * (disc * 4.0 + corona);
    fragColor = vec4(min(sky + sun, vec3(1.0)), 1.0);
}
