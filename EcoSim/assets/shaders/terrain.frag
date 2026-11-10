// Terrain fragment stage: diffuse lighting plus sand/grass/rock blends based on
// height and slope.
#version 330 core

in vec3 vWorldPos;
in vec3 vNormal;

uniform vec3 uLightDir;    // unit direction pointing FROM the light
uniform float uWaterLevel; // world-space height of the water plane
uniform float uMaxHeight;  // terrain generation maximum height

out vec4 FragColor;

void main() {
    vec3 normal = normalize(vNormal);
    vec3 light = normalize(-uLightDir);
    float diffuse = max(dot(normal, light), 0.0);

    // Sand fades in near the waterline; slopes and high ground fade to rock.
    float sandBlend =
        1.0 - smoothstep(uWaterLevel, uWaterLevel + 6.0, vWorldPos.y);
    float slopeRock = 1.0 - smoothstep(0.65, 0.8, normal.y);
    float heightRock =
        smoothstep(0.65 * uMaxHeight, 0.75 * uMaxHeight, vWorldPos.y);
    float rockBlend = max(slopeRock, heightRock);

    vec3 sand = vec3(0.76, 0.70, 0.50);
    vec3 grass = vec3(0.25, 0.55, 0.20);
    vec3 rock = vec3(0.45, 0.45, 0.45);
    vec3 baseColor = mix(grass, sand, sandBlend);
    baseColor = mix(baseColor, rock, rockBlend);

    vec3 color = baseColor * (0.3 + diffuse);
    FragColor = vec4(color, 1.0);
}
