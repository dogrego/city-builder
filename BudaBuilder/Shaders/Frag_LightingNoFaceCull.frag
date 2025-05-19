#version 430

// pipeline-ból bejövő per-fragment attribútumok
in vec3 worldPosition;
in vec3 worldNormal;
in vec2 textureCoords;

// kimenő érték - a fragment színe
out vec4 outputColor;

// textúra mintavételező objektum
uniform sampler2D textureImage;

uniform vec3 cameraPosition;

// Sun light properties
uniform vec4 lightPosition = vec4(0.0, 1.0, 0.0, 0.0);
uniform vec3 La = vec3(0.0, 0.0, 0.0);
uniform vec3 Ld = vec3(1.0, 1.0, 1.0);
uniform vec3 Ls = vec3(1.0, 1.0, 1.0);
uniform float lightConstantAttenuation = 1.0;
uniform float lightLinearAttenuation = 0.0;
uniform float lightQuadraticAttenuation = 0.0;

// Moon light properties
uniform vec4 moonLightPosition = vec4(0.0, -1.0, 0.0, 0.0);
uniform vec3 moonLa = vec3(0.05, 0.05, 0.1);
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

    // Set up sun light
    LightProperties sunLight;
    sunLight.pos = lightPosition;
    sunLight.La = La;
    sunLight.Ld = Ld;
    sunLight.Ls = Ls;
    sunLight.constantAttenuation = lightConstantAttenuation;
    sunLight.linearAttenuation = lightLinearAttenuation;
    sunLight.quadraticAttenuation = lightQuadraticAttenuation;

    // Set up moon light
    LightProperties moonLight;
    moonLight.pos = moonLightPosition;
    moonLight.La = moonLa;
    moonLight.Ld = moonLd;
    moonLight.Ls = moonLs;
    moonLight.constantAttenuation = 1.0; // No attenuation for directional light
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
    
    // Combine the lighting results (additive blending)
    vec3 shadedColor = sunShading + moonShading;
    
    // Apply texture
    outputColor = vec4(shadedColor, 1) * texture(textureImage, textureCoords);
}