// Fragment stage for debug line segments.
#version 330 core

in vec3 vColour;
out vec4 FragColor;

void main() {
    FragColor = vec4(vColour, 1.0);
}
