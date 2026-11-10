// Vertex stage for lit primitives: transforms positions and normals into world
// space and clip space.
#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;

uniform mat4 uModel;         // model matrix of the primitive
uniform mat4 uView;          // orbit-camera view matrix
uniform mat4 uProj;          // perspective projection
uniform mat3 uNormalMatrix;  // inverse-transpose of the model's upper-left 3x3

out vec3 vWorldPos;
out vec3 vNormal;

void main() {
    vec4 worldPosition = uModel * vec4(aPosition, 1.0);
    vWorldPos = worldPosition.xyz;
    vNormal = normalize(uNormalMatrix * aNormal);
    gl_Position = uProj * uView * worldPosition;
}
