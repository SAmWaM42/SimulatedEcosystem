// Blinn-Phong fragment stage for the primitive showcase.
#version 330 core

in vec3 vWorldPos;
in vec3 vNormal;

uniform vec3 uColor;      // base colour of the primitive
uniform vec3 uLightDir;   // unit direction pointing FROM the light
uniform vec3 uCameraPos;  // camera eye position for the specular term

out vec4 FragColor;

void main() {
    vec3 normal = normalize(vNormal);
    vec3 light = normalize(-uLightDir);
    vec3 view = normalize(uCameraPos - vWorldPos);
    vec3 halfway = normalize(light + view);

    float diffuse = max(dot(normal, light), 0.0);
    float specular = pow(max(dot(normal, halfway), 0.0), 32.0) * 0.25;
    vec3 color = uColor * (0.3 + diffuse) + vec3(specular);
    FragColor = vec4(color, 1.0);
}
