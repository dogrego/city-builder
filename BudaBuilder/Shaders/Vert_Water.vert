#version 430 core

layout(location = 0) in vec2 vs_in_uv;

out vec3 worldPosition;
out vec3 worldNormal;
out vec2 textureCoords;

uniform mat4 world;
uniform mat4 worldInvTransp;
uniform mat4 viewProj;
uniform float ElapsedTimeInSec = 0.0;
uniform vec3 cameraPosition;

const float TEXTURE_SCALE = 50.0;

// Wave parameters
const float WAVE_SPEED = 1.5;
const float WAVE_HEIGHT = 0.5;
const float WAVE_LENGTH = 5.0;
const float DETAIL_WAVE_SCALE = 0.3;

vec3 GetPos(float u, float v)
{
    // Convert UV to world space coordinates
    vec3 pos = vec3(-10.0, 0.0, 10.0) + vec3(20.0, 0.0, -20.0) * vec3(u, 0.0, v);
    
    // Main directional waves
    float wave1 = sin((pos.x + pos.z * 0.7 + ElapsedTimeInSec * WAVE_SPEED) / WAVE_LENGTH);
    float wave2 = cos((pos.z - pos.x * 0.5 + ElapsedTimeInSec * WAVE_SPEED * 0.8) / (WAVE_LENGTH * 0.7));
    
    // Detail waves
    float detailWave1 = sin((pos.x * 1.5 + pos.z * 2.0 + ElapsedTimeInSec * WAVE_SPEED * 2.5) / (WAVE_LENGTH * 0.3)) * DETAIL_WAVE_SCALE;
    float detailWave2 = cos((pos.z * 1.7 - pos.x * 1.2 + ElapsedTimeInSec * WAVE_SPEED * 1.7) / (WAVE_LENGTH * 0.4)) * DETAIL_WAVE_SCALE;
    
    // Combine waves
    pos.y = (wave1 + wave2) * WAVE_HEIGHT + detailWave1 + detailWave2;
    
    return pos;
}

vec3 GetNorm(float u, float v)
{
    // Calculate analytical derivatives of the wave function
    vec3 pos = vec3(-10.0, 0.0, 10.0) + vec3(20.0, 0.0, -20.0) * vec3(u, 0.0, v);
    
    // Main wave derivatives
    float dx = cos((pos.x + pos.z * 0.7 + ElapsedTimeInSec * WAVE_SPEED) / WAVE_LENGTH) / WAVE_LENGTH;
    dx += -0.5 * sin((pos.z - pos.x * 0.5 + ElapsedTimeInSec * WAVE_SPEED * 0.8) / (WAVE_LENGTH * 0.7)) / (WAVE_LENGTH * 0.7);
    
    // Detail wave derivatives
    dx += 1.5 * cos((pos.x * 1.5 + pos.z * 2.0 + ElapsedTimeInSec * WAVE_SPEED * 2.5) / (WAVE_LENGTH * 0.3)) / (WAVE_LENGTH * 0.3) * DETAIL_WAVE_SCALE;
    dx += -1.2 * sin((pos.z * 1.7 - pos.x * 1.2 + ElapsedTimeInSec * WAVE_SPEED * 1.7) / (WAVE_LENGTH * 0.4)) / (WAVE_LENGTH * 0.4) * DETAIL_WAVE_SCALE;
    
    float dz = 0.7 * cos((pos.x + pos.z * 0.7 + ElapsedTimeInSec * WAVE_SPEED) / WAVE_LENGTH) / WAVE_LENGTH;
    dz += sin((pos.z - pos.x * 0.5 + ElapsedTimeInSec * WAVE_SPEED * 0.8) / (WAVE_LENGTH * 0.7)) / (WAVE_LENGTH * 0.7);
    dz += 2.0 * cos((pos.x * 1.5 + pos.z * 2.0 + ElapsedTimeInSec * WAVE_SPEED * 2.5) / (WAVE_LENGTH * 0.3)) / (WAVE_LENGTH * 0.3) * DETAIL_WAVE_SCALE;
    dz += 1.7 * sin((pos.z * 1.7 - pos.x * 1.2 + ElapsedTimeInSec * WAVE_SPEED * 1.7) / (WAVE_LENGTH * 0.4)) / (WAVE_LENGTH * 0.4) * DETAIL_WAVE_SCALE;
    
    // Create normal from derivatives
    vec3 normal = normalize(vec3(-dx * WAVE_HEIGHT, 1.0, -dz * WAVE_HEIGHT));
    
    return normal;
}

void main()
{
    vec3 inputObjectSpacePosition = GetPos(vs_in_uv.x, vs_in_uv.y);
    gl_Position = viewProj * world * vec4(inputObjectSpacePosition, 1);
    
    worldPosition = (world * vec4(inputObjectSpacePosition, 1)).xyz;
    worldNormal = normalize((worldInvTransp * vec4(GetNorm(vs_in_uv.x, vs_in_uv.y), 0)).xyz);
    
    textureCoords = vs_in_uv * TEXTURE_SCALE;
}