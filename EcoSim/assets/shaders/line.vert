// Vertex stage for debug line segments.
#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aColour;

uniform mat4 uViewProj;  // combined view and projection matrix

out vec3 vColour;

void main() {
    vColour = aColour;
    gl_Position = uViewProj * vec4(aPosition, 1.0);
}
