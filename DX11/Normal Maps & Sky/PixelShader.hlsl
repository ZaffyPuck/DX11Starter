#include "HeaderStructs.hlsli"
#include "Lighting.hlsli"
#define LIGHT_AMOUNT 8 // 6 direct + 2 point
cbuffer ExternalData : register(b0)
{
    float3 colorTint;
    float3 cameraPosition;
    // Lighting
    float roughness;
    float3 ambientColor;
    Light lights[LIGHT_AMOUNT];
    // Texturing
    float2 uvScale;
    float2 uvOffset;
    int useSpecularMap;
}
Texture2D SurfaceTexture    : register(t0); // "t" registers for textures
Texture2D SpecularMap       : register(t1);
SamplerState BasicSampler   : register(s0); // "s" registers for samplers

// --------------------------------------------------------
// The entry point (main method) for our pixel shader
// 
// - Input is the data coming down the pipeline (defined by the struct)
// - Output is a single color (float4)
// - Has a special semantic (SV_TARGET), which means 
//    "put the output of this into the current render target"
// - Named "main" because that's the default the shader compiler looks for
// --------------------------------------------------------
float4 main(VertexToPixel input) : SV_TARGET
{
    input.normal = normalize(input.normal);
    
    input.uv = (input.uv * uvScale) + uvOffset;
    
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
        float3 dirToCamera = normalize(cameraPosition - input.worldPosition); // pixel world position
        
        switch (light.type)
        {
            case LIGHT_TYPE_DIRECTIONAL:
                pixelColor += DirectionalLight(light, input.normal, dirToCamera, roughness, surfaceColor, specularScale);
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

