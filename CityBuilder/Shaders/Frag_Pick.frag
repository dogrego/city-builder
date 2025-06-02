#version 430 core

in vec2 uv;

layout(location = 0) out vec3 outColor;

void main() {
    outColor = vec3(uv, 0.0);
}