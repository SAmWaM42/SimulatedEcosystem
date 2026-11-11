// Water fragment stage: animated normal ripples, moving specular glint,
// grazing-angle opacity, and distance fog.
#version 330 core

in vec3 vWorldPos;
in vec3 vNormal;

uniform vec3 uLightDir;    // unit direction pointing FROM the light
uniform vec3 uCameraPos;   // camera eye position for view and fog vectors
uniform float uTime;       // seconds used to scroll the ripple pattern
uniform vec3 uFogColor;    // sky/clear colour that distant water fades into
uniform float uFogDensity; // fog amount per world unit; app default 0.00012

out vec4 FragColor;

void main() {
    // Two scrolling sine pairs perturb the plane normal so the highlight moves
    // over time instead of sitting in one spot.
    vec3 normal = normalize(vNormal);
    vec2 worldXZ = vWorldPos.xz;
    normal.x += 0.045 * sin(worldXZ.x * 0.020 + uTime * 0.9) +
                0.025 * sin(worldXZ.y * 0.033 - uTime * 1.4);
    normal.z += 0.045 * cos(worldXZ.y * 0.021 - uTime * 1.1) +
                0.025 * cos(worldXZ.x * 0.037 + uTime * 1.7);
    normal = normalize(normal);

    vec3 light = normalize(-uLightDir);
    vec3 view = normalize(uCameraPos - vWorldPos);
    vec3 halfway = normalize(light + view);

    float diffuse = max(dot(normal, light), 0.0);
    float specular = pow(max(dot(normal, halfway), 0.0), 64.0) * 0.35;
    vec3 color = vec3(0.10, 0.35, 0.60) * (0.3 + diffuse) +
                 vec3(specular);

    // Facing the surface gives 0.45 opacity; grazing angles give 0.8.
    float facing = clamp(dot(normal, view), 0.0, 1.0);
    float waterAlpha = mix(0.8, 0.45, facing);

    // Exponential-squared distance fog; density is zero-safe.
    float distanceToCamera = length(uCameraPos - vWorldPos);
    float fogFactor =
        1.0 - exp(-pow(distanceToCamera * uFogDensity, 2.0));
    color = mix(color, uFogColor, clamp(fogFactor, 0.0, 1.0));
    FragColor = vec4(color, waterAlpha);
}
