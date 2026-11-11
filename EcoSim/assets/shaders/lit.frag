// Blinn-Phong fragment stage for lit organisms and primitives, with distance
// fog toward the sky colour.
#version 330 core

in vec3 vWorldPos;
in vec3 vNormal;

uniform vec3 uColor;      // base colour of the primitive
uniform vec3 uLightDir;   // unit direction pointing FROM the light
uniform vec3 uCameraPos;  // camera eye position for specular and fog distance
uniform vec3 uEmissive;   // added to the final colour to highlight a selection
uniform vec3 uFogColor;   // sky/clear colour that distant geometry fades into
uniform float uFogDensity; // fog amount per world unit; app default 0.00012

out vec4 FragColor;

void main() {
    vec3 normal = normalize(vNormal);
    vec3 light = normalize(-uLightDir);
    vec3 view = normalize(uCameraPos - vWorldPos);
    vec3 halfway = normalize(light + view);

    float diffuse = max(dot(normal, light), 0.0);
    float specular = pow(max(dot(normal, halfway), 0.0), 32.0) * 0.25;
    vec3 color = uColor * (0.3 + diffuse) + vec3(specular) + uEmissive;

    // Exponential-squared distance fog; density is zero-safe.
    float distanceToCamera = length(uCameraPos - vWorldPos);
    float fogFactor =
        1.0 - exp(-pow(distanceToCamera * uFogDensity, 2.0));
    color = mix(color, uFogColor, clamp(fogFactor, 0.0, 1.0));
    FragColor = vec4(color, 1.0);
}
