#include "HeaderStructs.hlsli"
#include "Lighting.hlsli"

cbuffer ExternalData : register(b0)
{
    // Lighting
    Light lights[MAX_LIGHTS];
    int lightCount;
    
    float3 ambientColor;
    float3 cameraPosition;
    
    // Texturing
    float3 colorTint;
    float2 uvScale;
    float2 uvOffset;
    
    int gammaCorrection;
    int useSpecularMap; // not used in here
    int useMetalMap;
    int useNormalMap;
    int useRoughnessMap;
    int useAlbedoTexture;
}
Texture2D Albedo : register(t0);
Texture2D NormalMap : register(t1);
Texture2D RoughnessMap : register(t2);
Texture2D MetalMap : register(t3);
SamplerState BasicSampler : register(s0);

float4 main(VertexToPixel input) : SV_TARGET
{
    input.normal = normalize(input.normal);
    input.tangent = normalize(input.tangent);
    input.uv = (input.uv * uvScale) + uvOffset;
    float roughness = 0.2f;
    float metal = 0.0f;
    float4 surfaceColor = Albedo.Sample(BasicSampler, input.uv);
    
    if (useNormalMap)
    {
        input.normal = NormalMapping(NormalMap, BasicSampler, input.uv, input.normal, input.tangent);
    }
    if (useRoughnessMap)
    {
        roughness = RoughnessMap.Sample(BasicSampler, input.uv).r; // non-PBR specular map
    }
    if (useMetalMap)
    {
        metal = MetalMap.Sample(BasicSampler, input.uv).r;
    }
    if (useAlbedoTexture)
    {
        if (gammaCorrection)
        {
            surfaceColor.rgb = pow(surfaceColor.rgb, 2.2);
        }
    }
    else
    {
        surfaceColor.rgb = colorTint.rgb;
    }

    // Specular color determination -----------------
    // Assume albedo texture is actually holding specular color where metalness == 1
    // Note the use of lerp here - metal is generally 0 or 1, but might be in between
    // because of linear texture sampling, so we lerp the specular color to match
    float3 specularColor = lerp(F0_NON_METAL, surfaceColor.rgb, metal);
    float3 pixelColor = ambientColor * surfaceColor.rgb;
    
    for (int i = 0; i < lightCount; i++)
    {
        Light light = lights[i];
        light.direction = normalize(light.direction); // away from light source
        
        switch (light.type)
        {
            case LIGHT_TYPE_DIRECTIONAL:
                pixelColor += DirLightPBR(light, input.normal, input.worldPosition, cameraPosition, roughness, metal, surfaceColor.rgb, specularColor);
                break;
            case LIGHT_TYPE_POINT:
                pixelColor += PointLightPBR(light, input.normal, input.worldPosition, cameraPosition, roughness, metal, surfaceColor.rgb, specularColor);
                break;
            case LIGHT_TYPE_SPOT:
                pixelColor += SpotLightPBR(light, input.normal, input.worldPosition, cameraPosition, roughness, metal, surfaceColor.rgb, specularColor);
                break;
        }
    }
    
    if (gammaCorrection)
    {
        pixelColor = pow(pixelColor, 1.0f / 2.2f);
    }
    return float4(pixelColor, 1);
    
}

