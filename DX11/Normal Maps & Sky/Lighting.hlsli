#ifndef __GGP_SHADER_INCLUDES__
#define __GGP_SHADER_INCLUDES__
#define LIGHT_TYPE_DIRECTIONAL 0
#define LIGHT_TYPE_POINT 1
#define LIGHT_TYPE_SPOT 2
#define MAX_SPECULAR_EXPONENT 256.0f

struct Light
{
    int type;
    float3 direction;
    float range;
    float3 position;
    float intensity;
    float3 color;
    float spotFalloff;
    float3 padding;
};

// === CONSTANTS === //
static const float PI = 3.14159265359f;
static const float TWO_PI = PI * 2.0f;
static const float HALF_PI = PI / 2.0f;
static const float QUARTER_PI = PI / 4.0f;

// === UTILITY FUNCTIONS === //
// Handle converting tangent-space normal map to world space normal
float3 NormalMapping(Texture2D map, SamplerState samp, float2 uv, float3 normal, float3 tangent)
{
	// Grab the normal from the map
    float3 normalFromMap = map.Sample(samp, uv).rgb * 2.0f - 1.0f;

	// Gather the required vectors for converting the normal
    float3 N = normal;
    float3 T = normalize(tangent - N * dot(tangent, N));
    float3 B = cross(T, N);

	// Create the 3x3 matrix to convert from TANGENT-SPACE normals to WORLD-SPACE normals
    float3x3 TBN = float3x3(T, B, N);

	// Adjust the normal from the map and simply use the results
    return normalize(mul(normalFromMap, TBN));
}
// Range-based attenuation function
float Attenuate(Light light, float3 worldPos)
{
    float dist = distance(light.position, worldPos); // Calculate the distance between the surface and the light
    float att = saturate(1.0f - (dist * dist / (light.range * light.range))); // Ranged-based attenuation
    return att * att; // Soft falloff
}

// === BASIC LIGHTING ==== //
// Lambert diffuse BRDF
float Diffuse(float3 normal, float3 dirToLight)
{
    float dotProd = dot(normal, dirToLight);
    return saturate(dotProd); // disallows negatives (clamps to [0,1])
}
// Phong (specular) BRDF
float SpecularPhong(float3 normal, float3 dirToLight, float roughness, float3 dirToCamera)
{
    float3 reflection = reflect(-dirToLight, normal); // Calculate reflection vector
    float specExponent = (1 - roughness) * MAX_SPECULAR_EXPONENT;
    if (roughness == 1)
    {
        return 0.0f;
    }
    return pow(max(dot(dirToCamera, reflection), 0), specExponent); // Compare reflection vector & view vector and raise to a power
}
// Blinn-Phong (specular) BRDF
float SpecularBlinnPhong(float3 normal, float3 dirToLight, float3 toCamera, float roughness)
{
	// Calculate halfway vector
    float3 halfwayVector = normalize(dirToLight + toCamera);
    float specExponent = (1 - roughness) * MAX_SPECULAR_EXPONENT;

	// Compare halflway vector & normal and raise to a power
    return roughness == 1 ? 0.0f : pow(max(dot(halfwayVector, normal), 0), specExponent);
}

// === LIGHT TYPES FOR BASIC LIGHTING === //
float3 DirectionalLight(Light light, float3 normal, float3 directionToCamera, float roughness, float3 surfaceColor, float specularScale = 1.0f)
{
    float3 dirToLightSource = -light.direction; // should already be normalized
    float diffusion = Diffuse(normal, dirToLightSource);
    float specular = SpecularPhong(normal, dirToLightSource, roughness, directionToCamera);
    specular *= specularScale;

    // Cut the specular if the diffuse contribution is zero
    // - any() returns 1 if any component of the param is non-zero
    // - In this case, diffuse is a single float value
    // - Meaning any() returns 1 if diffuse itself is non-zero
    // - In other words:
    // - If the diffuse amount is 0, any(diffuse) returns 0
    // - If the diffuse amount is != 0, any(diffuse) returns 1
    // - So when diffuse is 0, specular becomes 0   
    specular *= any(diffusion);
    
    float3 finalColor = surfaceColor * diffusion + specular; // Don’t tint specular? // //float3 light = colorTint * (diffuse + specular); // Tint specular?
    
	// Combine
    return finalColor * light.intensity * light.color;
}
float3 PointLight(Light light, float3 normal, float3 worldPos, float3 camPos, float roughness, float3 surfaceColor, float specularScale = 1.0f)
{
	// Calc light direction
    float3 dirToLightSource = normalize(light.position - worldPos);
    float3 toCam = normalize(camPos - worldPos);

	// Calculate the light amounts
    float attenuation = Attenuate(light, worldPos);
    float diffusion = Diffuse(normal, dirToLightSource);
    float specular = SpecularPhong(normal, dirToLightSource, roughness, toCam);
    specular *= specularScale;
    specular *= any(diffusion);

    float3 finalColor = surfaceColor * diffusion + specular;
    return finalColor * attenuation * light.intensity * light.color;
}
float3 SpotLight(Light light, float3 normal, float3 worldPos, float3 camPos, float roughness, float3 surfaceColor, float specularScale)
{
    float3 dirToLightSource = normalize(light.position - worldPos);
    float penumbra = pow(saturate(dot(-dirToLightSource, light.direction)), light.spotFalloff);
    return PointLight(light, normal, worldPos, camPos, roughness, surfaceColor, specularScale) * penumbra;
}
#endif