#include "HeaderStructs.hlsli"
#include "Lighting.hlsli"
#define LIGHT_AMOUNT 8 // 6 direct + 2 point
cbuffer ExternalData : register(b0)
{
    float3 colorTint;
    float roughness;
    float3 ambientColor;
    float3 cameraPosition;
    Light lights[LIGHT_AMOUNT];
    float2 uvScale;
    float2 uvOffset;
    int useSpecularMap;
}
Texture2D SurfaceTexture : register(t0); // "t" registers for textures
Texture2D SpecularMap : register(t1);
Texture2D NormalMap : register(t2);
SamplerState BasicSampler : register(s0); // "s" registers for samplers

float4 main(VertexToPixel input) : SV_TARGET
{
    input.uv = (input.uv * uvScale) + uvOffset;
    
    // Normal/tangent calculations
    float3 unpackedNormal = NormalMap.Sample(BasicSampler, input.uv).rgb * 2.0f - 1.0f; // [0,1] to [-1,1]
    unpackedNormal = normalize(unpackedNormal);
    // Gram-Schmidt orthonormalize process
    float3 N = normalize(input.normal);
    float3 T = normalize(input.tangent);
    T = normalize(T - N * dot(T, N)); // Gram-Schmidt assumes T&N are normalized
    //return float4(T, 1);
    // Get bi-tangent
    float3 B = cross(T, N);
    float3x3 TBN = float3x3(T, B, N);
    // Transform unpacked normals
    input.normal = normalize(mul(unpackedNormal, TBN)); // Adjust the normal from the map and simply use the results
    
    float3 surfaceColor = SurfaceTexture.Sample(BasicSampler, input.uv).rgb;
    float3 pixelColor = ambientColor * (colorTint * surfaceColor);
    
    float specularScale = 1.0f;
    if (useSpecularMap)
    {
        specularScale = SpecularMap.Sample(BasicSampler, input.uv).r;
    }
    
    for (int i = 0; i < LIGHT_AMOUNT; i++)
    {
        Light light = lights[i];
        light.direction = normalize(light.direction); // away from light source
        
        switch (light.type)
        {
            case LIGHT_TYPE_DIRECTIONAL:
                pixelColor += DirLight(light, input.normal, input.worldPosition, cameraPosition, roughness, surfaceColor, specularScale);
                break;
            case LIGHT_TYPE_POINT:
                pixelColor += PointLight(light, input.normal, input.worldPosition, cameraPosition, roughness, surfaceColor, specularScale);
                break;
            case LIGHT_TYPE_SPOT:
                pixelColor += SpotLight(light, input.normal, input.worldPosition, cameraPosition, roughness, surfaceColor, specularScale);
                break;
        }
    }
    
    return float4(pixelColor, 1);
}

