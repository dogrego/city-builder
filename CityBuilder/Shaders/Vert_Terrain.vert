#version 450 core

layout(location = 0) in vec2 vtxUV;

uniform mat4 world;
uniform mat4 viewProj;
uniform sampler2D heightmap;
uniform float heightScale;
uniform float verticalOffset; // New uniform for vertical movement

out vec2 texCoord;
out vec3 worldPos;
out vec3 worldNormal;
out float heightValue;

void main() {
    // Sample heightmap (unchanged)
    heightValue = texture(heightmap, vtxUV).r;
    
    // Calculate world position with vertical offset
    vec3 pos = vec3(
        vtxUV.x - 0.5, 
        (heightValue * heightScale - heightScale/2.0) + verticalOffset, // Add offset here
        vtxUV.y - 0.5
    );
    
    worldPos = (world * vec4(pos, 1.0)).xyz;
    
    // Normal calculation remains unchanged
    float hL = textureOffset(heightmap, vtxUV, ivec2(-1, 0)).r;
    float hR = textureOffset(heightmap, vtxUV, ivec2(1, 0)).r;
    float hD = textureOffset(heightmap, vtxUV, ivec2(0, -1)).r;
    float hU = textureOffset(heightmap, vtxUV, ivec2(0, 1)).r;
    
    vec3 normal = normalize(vec3(hL - hR, 2.0, hD - hU));
    worldNormal = normalize(mat3(world) * normal);
    
    texCoord = vtxUV;
    gl_Position = viewProj * vec4(worldPos, 1.0);
}