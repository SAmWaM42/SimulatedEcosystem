// Terrain fragment stage: hemispheric ambient, green land with sandy shores,
// procedural albedo variation, and distance fog.
#version 330 core

in vec3 vWorldPos;
in vec3 vNormal;

uniform vec3 uLightDir;    // unit direction pointing FROM the light
uniform vec3 uCameraPos;   // camera eye position for fog distance
uniform float uWaterLevel; // world-space height of the water plane
uniform vec3 uFogColor;    // sky/clear colour that distant terrain fades into
uniform float uFogDensity; // fog amount per world unit; app default 0.00012

out vec4 FragColor;

// Hash and smooth value noise used only for small albedo variation.
float hash21(vec2 p) {
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}

float valueNoise(vec2 p) {
    vec2 cell = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    float a = hash21(cell);
    float b = hash21(cell + vec2(1.0, 0.0));
    float c = hash21(cell + vec2(0.0, 1.0));
    float d = hash21(cell + vec2(1.0, 1.0));
    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

void main() {
    vec3 normal = normalize(vNormal);
    vec3 light = normalize(-uLightDir);
    float diffuse = max(dot(normal, light), 0.0);

    // Sky above and warm bounce from below give slopes a softer ambient term.
    vec3 skyAmbient = vec3(0.38, 0.48, 0.58);
    vec3 groundAmbient = vec3(0.16, 0.13, 0.10);
    float skyWeight = clamp(normal.y * 0.5 + 0.5, 0.0, 1.0);
    vec3 ambient = mix(groundAmbient, skyAmbient, skyWeight);

    // Sand fades in near the waterline; every other land height stays green.
    float sandBlend =
        1.0 - smoothstep(uWaterLevel, uWaterLevel + 6.0, vWorldPos.y);

    vec3 sand = vec3(0.76, 0.70, 0.50);
    vec3 grass = vec3(0.25, 0.55, 0.20);
    vec3 baseColor = mix(grass, sand, sandBlend);

    // Two octaves of world-space value noise vary the albedo by about +/-8%.
    float variation = valueNoise(vWorldPos.xz * 0.020) * 0.65 +
                      valueNoise(vWorldPos.xz * 0.061 + 17.0) * 0.35;
    baseColor *= 0.92 + 0.16 * variation;

    vec3 color =
        baseColor * (ambient + diffuse * vec3(0.95, 0.92, 0.82));

    // Exponential-squared distance fog; density is zero-safe.
    float distanceToCamera = length(uCameraPos - vWorldPos);
    float fogFactor =
        1.0 - exp(-pow(distanceToCamera * uFogDensity, 2.0));
    color = mix(color, uFogColor, clamp(fogFactor, 0.0, 1.0));
    FragColor = vec4(color, 1.0);
}
