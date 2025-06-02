#version 430 core

layout(location = 0) in vec2 vtxPos;

uniform mat4 world;
uniform mat4 viewProj;

out vec2 uv;

void main() {
    uv = vtxPos;
    gl_Position = viewProj * world * vec4(vtxPos.x, 0.0, vtxPos.y, 1.0);
}