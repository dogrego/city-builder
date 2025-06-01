#version 430

in vec3 worldPosition;
in vec3 worldNormal;
in vec2 textureCoords;

out vec4 outputColor;

uniform sampler2D textureImage;
uniform vec3 cameraPosition;

// Sun light properties
uniform vec4 lightPosition = vec4(0.0, 1.0, 0.0, 0.0);
uniform vec3 La = vec3(0.4, 0.4, 0.5);  // Sun ambient (used for day ambient)
uniform vec3 Ld = vec3(1.0, 1.0, 1.0);
uniform vec3 Ls = vec3(1.0, 1.0, 1.0);

// Moon light properties
uniform vec4 moonLightPosition = vec4(0.0, -1.0, 0.0, 0.0);
uniform vec3 moonLa = vec3(0.05, 0.05, 0.1);  // Moon ambient (used for night ambient)
uniform vec3 moonLd = vec3(0.2, 0.2, 0.3);
uniform vec3 moonLs = vec3(0.3, 0.3, 0.4);

// material properties
uniform vec3 Ka = vec3(1.0);
uniform vec3 Kd = vec3(1.0);
uniform vec3 Ks = vec3(1.0);
uniform float Shininess = 1.0;

struct LightProperties
{
    vec4 pos;
    vec3 La;
    vec3 Ld;
    vec3 Ls;
    float constantAttenuation;
    float linearAttenuation;
    float quadraticAttenuation;
};

struct MaterialProperties
{
    vec3 Ka;
    vec3 Kd;
    vec3 Ks;
    float Shininess;
};

vec3 lighting(LightProperties light, vec3 position, vec3 normal, MaterialProperties material)
{
    vec3 ToLight;
    float LightDistance = 0.0;
    
    if (light.pos.w == 0.0) // directional light
    {
        ToLight = light.pos.xyz;
    }
    else // point light
    {
        ToLight = light.pos.xyz - position;
        LightDistance = length(ToLight);
    }
    
    ToLight = normalize(ToLight);
    
    float Attenuation = 1.0 / (light.constantAttenuation + 
                              light.linearAttenuation * LightDistance + 
                              light.quadraticAttenuation * LightDistance * LightDistance);
    
    // Ambient component
    vec3 Ambient = light.La * material.Ka;

    // Diffuse component
    float DiffuseFactor = max(dot(ToLight, normal), 0.0) * Attenuation;
    vec3 Diffuse = DiffuseFactor * light.Ld * material.Kd;
    
    // Specular component
    vec3 viewDir = normalize(cameraPosition - position);
    vec3 reflectDir = reflect(-ToLight, normal);
    float SpecularFactor = pow(max(dot(viewDir, reflectDir), 0.0), material.Shininess) * Attenuation;
    vec3 Specular = SpecularFactor * light.Ls * material.Ks;

    return Ambient + Diffuse + Specular;
}

void main()
{
    // Normalize the fragment normal
    vec3 normal = normalize(worldNormal);
    if (!gl_FrontFacing)
        normal = -normal;

    // Determine if it's day or night based on sun position
    bool isDay = lightPosition.y > -0.2; // Sun is above horizon
    
    // Use sun's La for day ambient and moon's La for night ambient
    vec3 globalAmbient = isDay ? La : moonLa;

    // Set up sun light
    LightProperties sunLight;
    sunLight.pos = lightPosition;
    sunLight.La = vec3(0.0); // We're using global ambient instead
    sunLight.Ld = Ld;
    sunLight.Ls = Ls;
    sunLight.constantAttenuation = 1.0;
    sunLight.linearAttenuation = 0.0;
    sunLight.quadraticAttenuation = 0.0;

    // Set up moon light (only active at night)
    LightProperties moonLight;
    moonLight.pos = moonLightPosition;
    moonLight.La = vec3(0.0); // We're using global ambient instead
    moonLight.Ld = isDay ? vec3(0.0) : moonLd;
    moonLight.Ls = isDay ? vec3(0.0) : moonLs;
    moonLight.constantAttenuation = 1.0;
    moonLight.linearAttenuation = 0.0;
    moonLight.quadraticAttenuation = 0.0;

    // Material properties
    MaterialProperties material;
    material.Ka = Ka;
    material.Kd = Kd;
    material.Ks = Ks;
    material.Shininess = Shininess;

    // Calculate lighting from both sources
    vec3 sunShading = lighting(sunLight, worldPosition, normal, material);
    vec3 moonShading = lighting(moonLight, worldPosition, normal, material);
    
    // Combine the lighting results with global ambient
    vec3 shadedColor = globalAmbient * material.Ka + sunShading + moonShading;
    
    // Apply texture
    vec4 texColor = texture(textureImage, textureCoords);
    outputColor = vec4(shadedColor, 1.0) * texColor;
    
    // Ensure minimum visibility even at night
    float minVisibility = isDay ? 0.15 : 0.05;
    outputColor.rgb = max(outputColor.rgb, vec3(minVisibility) * texColor.rgb);
}