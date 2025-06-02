#version 430 core

in vec2 uv;

layout(location = 0) out vec3 outColor;

uniform int buildingID;

void main() {
    outColor = vec3(uv, float(buildingID + 1)/255.0);
}