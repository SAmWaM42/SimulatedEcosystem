// Water fragment stage: flat blue colour, alpha blending, and a small
// specular highlight.
#version 330 core

in vec3 vWorldPos;
in vec3 vNormal;

uniform vec3 uLightDir;   // unit direction pointing FROM the light
uniform vec3 uCameraPos;  // camera eye position for the specular term

out vec4 FragColor;

void main() {
    const vec3 waterColor = vec3(0.10, 0.35, 0.60);
    const float waterAlpha = 0.55;

    vec3 normal = normalize(vNormal);
    vec3 light = normalize(-uLightDir);
    vec3 view = normalize(uCameraPos - vWorldPos);
    vec3 halfway = normalize(light + view);

    float diffuse = max(dot(normal, light), 0.0);
    float specular = pow(max(dot(normal, halfway), 0.0), 64.0) * 0.2;
    vec3 color = waterColor * (0.3 + diffuse) + vec3(specular);
    FragColor = vec4(color, waterAlpha);
}
