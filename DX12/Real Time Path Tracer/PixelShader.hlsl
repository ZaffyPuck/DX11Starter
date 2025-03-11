#include "ShaderStructs.hlsli"
#include "Lighting.hlsli"

cbuffer ExternalData : register(b0)
{
    // Texturing
    float2 uvScale;
    float2 uvOffset;
    // General
    float3 cameraPosition;
    // Lighting
    int lightCount;
    Light lights[MAX_LIGHTS];
}

Texture2D AlbedoTexture : register(t0);
Texture2D NormalMap     : register(t1);
Texture2D RoughnessMap  : register(t2);
Texture2D MetalMap      : register(t3);
SamplerState Sampler    : register(s0);

float4 main(VertexToPixel input) : SV_TARGET
{
    float4 surfaceColor = AlbedoTexture.Sample(Sampler, input.uv);
    surfaceColor.rgb = pow(surfaceColor.rgb, 2.2);

    input.normal = normalize(input.normal);
    input.normal = NormalMapping(NormalMap, Sampler, input.uv, input.normal, input.tangent);

    input.tangent = normalize(input.tangent);

    input.uv = (input.uv * uvScale) + uvOffset;

    float roughness = 0.2f;
    roughness = RoughnessMap.Sample(Sampler, input.uv).r; // non-PBR specular map

    float metal = 0.0f;
    metal = MetalMap.Sample(Sampler, input.uv).r;

    float3 specularColor = lerp(F0_NON_METAL, surfaceColor.rgb, metal);
    float3 ambientColor = float3(0.1, 0.1, 0.2); // should be passed in?
    float3 pixelColor = ambientColor * surfaceColor.rgb; // ambientColor

    for (int i = 0; i < lightCount; i++)
    {
        Light light = lights[i];
        light.direction = normalize(light.direction); // away from light source

        switch (light.type)
        {
            case LIGHT_TYPE_DIRECTIONAL:
                pixelColor += DirLightPBR(light, input.normal, input.positionWorld, cameraPosition, roughness, metal, surfaceColor.rgb, specularColor);
                break;
            case LIGHT_TYPE_POINT:
                pixelColor += PointLightPBR(light, input.normal, input.positionWorld, cameraPosition, roughness, metal, surfaceColor.rgb, specularColor);
                break;
            case LIGHT_TYPE_SPOT:
                pixelColor += SpotLightPBR(light, input.normal, input.positionWorld, cameraPosition, roughness, metal, surfaceColor.rgb, specularColor);
                break;
        }
    }

    pixelColor = pow(pixelColor, 1.0f / 2.2f); // I think we undo it so the light calculations are not affected

    return float4(pixelColor, 1); // lights dont appear to be working
}