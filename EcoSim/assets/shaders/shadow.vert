// Depth-only vertex stage for the virtual-sun shadow pass.
#version 330 core

layout(location = 0) in vec3 aPosition;

uniform mat4 uLightSpaceMatrix; // world -> orthographic light clip space
uniform mat4 uModel;            // per-primitive model transform

void main() {
    gl_Position = uLightSpaceMatrix * uModel * vec4(aPosition, 1.0);
}
