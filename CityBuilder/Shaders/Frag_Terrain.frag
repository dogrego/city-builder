#version 450 core

in vec2 texCoord;
in vec3 worldPos;
in vec3 worldNormal;
in float heightValue;

uniform sampler2D splatmap;
uniform sampler2D groundTextures[4];
uniform sampler2D rockTexture;
uniform sampler2D sandTexture;
uniform sampler2D snowTexture;
uniform float texScale;

// Lighting uniforms - simplified
uniform vec3 cameraPosition;
uniform vec4 lightPosition;
uniform vec3 La;
uniform vec3 Ld;
uniform vec4 moonLightPosition;
uniform vec3 moonLa;
uniform vec3 moonLd;
uniform vec3 Ka;
uniform vec3 Kd;

out vec4 fragColor;

void main() {
    // Sample splatmap to get texture weights
    vec4 weights = texture(splatmap, texCoord);
    
    // Normalize weights
    float totalWeight = weights.r + weights.g + weights.b + weights.a;
    weights /= max(totalWeight, 0.001);
    
    // Sample textures with different scales
    vec3 tex0 = texture(groundTextures[0], texCoord * texScale * 2.0).rgb;
    vec3 tex1 = texture(groundTextures[1], texCoord * texScale * 1.0).rgb;
    vec3 tex2 = texture(groundTextures[2], texCoord * texScale * 0.5).rgb;
    vec3 tex3 = texture(groundTextures[3], texCoord * texScale * 0.25).rgb;
    
    // Blend ground textures
    vec3 baseColor = tex0 * weights.r + tex1 * weights.g + tex2 * weights.b + tex3 * weights.a;
    
    // Calculate steepness (1.0 = vertical cliff, 0.0 = completely flat)
float steepness = 1.0 - dot(worldNormal, vec3(0.0, 1.0, 0.0));

const float rockThreshold = 0.125;
const float rockBlendRange = 0.07; // Small blend range for sharp transition

// Calculate rock factor with proper threshold
float rockFactor = smoothstep(rockThreshold - rockBlendRange, 
                             rockThreshold + rockBlendRange, 
                             steepness);

// Alternative for ultra-sharp cutoff (no blending):
// float rockFactor = step(rockThreshold, steepness);

// Apply rock texture
vec3 rockTex = texture(rockTexture, texCoord * texScale * 3.0).rgb;
baseColor = mix(baseColor, rockTex, rockFactor);
    
    // Add snow at higher altitudes
    float snowFactor = smoothstep(0.62, 0.65, heightValue);
    vec3 snowTex = texture(snowTexture, texCoord * texScale * 2.0).rgb;
    baseColor = mix(baseColor, snowTex, snowFactor);
    
    // Limit sand to low and flat areas
    float sandFactor = smoothstep(0.5, 0.3, heightValue);

    vec3 sandTex = texture(sandTexture, texCoord * texScale).rgb;
    baseColor = mix(baseColor, sandTex, sandFactor);

    
    // Simplified lighting - diffuse only
    vec3 lightDir = normalize(lightPosition.xyz - worldPos * lightPosition.w);
    vec3 moonLightDir = normalize(moonLightPosition.xyz - worldPos * moonLightPosition.w);
    
    // Sun lighting (diffuse only)
    float diff = max(dot(worldNormal, lightDir), 0.0);
    
    // Moon lighting (diffuse only)
    float moonDiff = max(dot(worldNormal, moonLightDir), 0.0) * 0.5;
    
    // Combine lighting (no specular)
    vec3 ambient = La * Ka;
    vec3 diffuse = Ld * Kd * diff;
    vec3 moonAmbient = moonLa * Ka * 0.5;
    vec3 moonDiffuse = moonLd * Kd * moonDiff;
    
    vec3 litColor = baseColor * (ambient + diffuse + moonAmbient + moonDiffuse);
    
    fragColor = vec4(litColor, 1.0);
}