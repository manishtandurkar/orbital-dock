#version 330 core

const int kMaxPointLights = 8;

struct DirectionalLight {
    vec3 direction;
    vec3 color;
    float intensity;
};

struct PointLight {
    vec3 position;
    vec3 color;
    float intensity;
    float range;
};

in vec3 vWorldPosition;
in vec3 vNormal;
in vec2 vUv;
in vec4 vLightSpacePosition;

uniform DirectionalLight uSun;
uniform PointLight uPointLights[kMaxPointLights];
uniform int uPointLightCount;
uniform vec3 uAmbientColor;
uniform vec3 uViewPosition;
uniform vec3 uFillDirection;
uniform vec3 uFillColor;
uniform bool uShadowsEnabled;
uniform sampler2DShadow uShadowMap;
uniform float uShadowTexelSize;

uniform vec3 uDiffuseColor;
uniform vec3 uSpecularColor;
uniform float uShininess;
uniform vec3 uEmissiveColor;
uniform float uEmissiveStrength;
uniform vec3 uTint;
uniform bool uUseDiffuseMap;
uniform sampler2D uDiffuseMap;
uniform bool uUnlit;
uniform vec3 uRimColor;
uniform float uRimStrength;

out vec4 fragColor;

vec3 toLinear(vec3 color)
{
    return pow(color, vec3(2.2));
}

vec3 blinnPhong(vec3 normal, vec3 viewDirection, vec3 lightDirection, vec3 radiance, vec3 albedo)
{
    float diffuseTerm = max(dot(normal, lightDirection), 0.0);
    vec3 halfway = normalize(lightDirection + viewDirection);
    float specularTerm = diffuseTerm > 0.0 ? pow(max(dot(normal, halfway), 0.0), uShininess) : 0.0;
    float energyNormalisation = (uShininess + 8.0) / 25.13274;
    return radiance * (albedo * diffuseTerm + uSpecularColor * specularTerm * energyNormalisation * 0.25);
}

float sunVisibility(vec3 normal, vec3 sunDirection)
{
    if (!uShadowsEnabled) {
        return 1.0;
    }
    vec3 projected = vLightSpacePosition.xyz / vLightSpacePosition.w * 0.5 + 0.5;
    if (projected.x <= 0.0 || projected.x >= 1.0 || projected.y <= 0.0 || projected.y >= 1.0 || projected.z >= 1.0) {
        return 1.0;
    }
    float slope = 1.0 - max(dot(normal, sunDirection), 0.0);
    float bias = 0.0008 + 0.0025 * slope;
    float lit = 0.0;
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            vec2 offset = vec2(float(x), float(y)) * uShadowTexelSize;
            lit += texture(uShadowMap, vec3(projected.xy + offset, projected.z - bias));
        }
    }
    return lit / 9.0;
}

float rangeAttenuation(float distanceToLight, float range)
{
    float ratio = distanceToLight / range;
    float window = clamp(1.0 - ratio * ratio * ratio * ratio, 0.0, 1.0);
    return window * window / (1.0 + distanceToLight * distanceToLight * 0.02);
}

void main()
{
    vec3 albedo = toLinear(uDiffuseColor * uTint);
    if (uUseDiffuseMap) {
        albedo *= toLinear(texture(uDiffuseMap, vUv).rgb);
    }
    vec3 emissive = toLinear(uEmissiveColor) * uEmissiveStrength;

    vec3 color;
    if (uUnlit) {
        color = albedo + emissive;
    } else {
        vec3 normal = normalize(vNormal);
        vec3 viewDirection = normalize(uViewPosition - vWorldPosition);
        if (dot(normal, viewDirection) < 0.0) {
            normal = -normal;
        }

        color = toLinear(uAmbientColor) * albedo;
        vec3 sunDirection = normalize(-uSun.direction);
        float sunlight = sunVisibility(normal, sunDirection);
        color += blinnPhong(normal, viewDirection, sunDirection, toLinear(uSun.color) * uSun.intensity, albedo) * sunlight;
        color += albedo * toLinear(uFillColor) * max(dot(normal, -normalize(uFillDirection)), 0.0);

        for (int i = 0; i < uPointLightCount && i < kMaxPointLights; ++i) {
            vec3 toLight = uPointLights[i].position - vWorldPosition;
            float distanceToLight = length(toLight);
            if (distanceToLight >= uPointLights[i].range) {
                continue;
            }
            vec3 lightDirection = toLight / max(distanceToLight, 0.0001);
            float attenuation = rangeAttenuation(distanceToLight, uPointLights[i].range);
            vec3 radiance = toLinear(uPointLights[i].color) * uPointLights[i].intensity * attenuation;
            color += blinnPhong(normal, viewDirection, lightDirection, radiance, albedo);
        }
        color += emissive;

        if (uRimStrength > 0.0) {
            float rim = pow(1.0 - max(dot(normal, viewDirection), 0.0), 3.0);
            float sunFacing = 0.25 + 0.75 * max(dot(normal, sunDirection), 0.0);
            color += toLinear(uRimColor) * rim * sunFacing * uRimStrength;
        }
    }

    fragColor = vec4(color, 1.0);
}
