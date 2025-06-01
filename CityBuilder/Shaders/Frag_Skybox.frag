#version 430 core

in vec3 worldPosition;
out vec4 fragColor;

uniform vec3 sunDirection;
uniform vec3 moonDirection;
uniform vec3 sunColor;
uniform vec3 moonColor;
uniform vec3 skyTopColor;
uniform vec3 skyBottomColor;

void main()
{
    // Normalized position from -1 to 1
    vec3 pos = normalize(worldPosition);
    
    // Sky gradient - darker at bottom
    float gradient = smoothstep(-1.0, 1.0, pos.y);
    vec3 skyColor = mix(skyBottomColor, skyTopColor, gradient);
    
    // Sun disc with smooth edges
    float sunAngle = acos(dot(pos, sunDirection));
    float sunSize = 0.05;
    float sun = smoothstep(sunSize, sunSize * 0.8, sunAngle);
    
    // Moon disc with smooth edges
    float moonAngle = acos(dot(pos, moonDirection));
    float moonSize = 0.05;
    float moon = smoothstep(moonSize, moonSize * 0.8, moonAngle);
    
    // Sun glow effect
    float sunGlow = 1.0 - smoothstep(0.0, sunSize * 2.0, sunAngle);
    skyColor = mix(skyColor, sunColor, sunGlow * 0.3);
    
    // Combine everything
    vec3 color = skyColor;
    color = mix(color, sunColor, sun);
    color = mix(color, moonColor, moon);
    
    fragColor = vec4(color, 1.0);
}