#version 430 core

layout( location = 0 ) in vec2 vs_in_uv;

// Values to pass through the pipeline
out vec3 worldPosition;
out vec3 worldNormal;
out vec2 textureCoords;

// External shader parameters
uniform mat4 world;
uniform mat4 worldInvTransp;
uniform mat4 viewProj;

uniform float ElapsedTimeInSec = 0.0;

vec3 GetPos(float u, float v)
{
    vec3 pos = vec3(-10.0, 0.0, 10.0) + vec3(20.0, 0.0, -20.0) * vec3(u, 0.0, v);
    pos.y = sin(pos.z + ElapsedTimeInSec * 2.0); // Increased wave speed by multiplying time
    
    // Add more complex wave pattern
    pos.y += 0.3 * sin(pos.x * 0.5 + ElapsedTimeInSec * 1.5);
    pos.y += 0.2 * cos(pos.z * 0.3 + ElapsedTimeInSec * 1.2);
    
    return pos;
}

vec3 GetNorm(float u, float v)
{
    vec3 du = GetPos(u + 0.01, v) - GetPos(u - 0.01, v);
    vec3 dv = GetPos(u, v + 0.01) - GetPos(u, v - 0.01);

    return normalize(cross(du, dv));
}

void main()
{
    vec3 inputObjectSpacePosition = GetPos(vs_in_uv.x, vs_in_uv.y);
    gl_Position = viewProj * world * vec4(inputObjectSpacePosition, 1);
    
    worldPosition = (world * vec4(inputObjectSpacePosition, 1)).xyz;
    worldNormal = (worldInvTransp * vec4(GetNorm(vs_in_uv.x, vs_in_uv.y), 0)).xyz;
    textureCoords = vs_in_uv;
}