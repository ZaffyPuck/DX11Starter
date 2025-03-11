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
Texture2D ShadowMap : register(t4);
SamplerState BasicSampler : register(s0);
SamplerComparisonState ShadowSampler : register(s1);

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
    float3 pixelColor = ambientColor * surfaceColor.rgb; //  get rid of ambient color?
    
    // Perform the perspective divide (divide by W) ourselves
    input.shadowMapPos /= input.shadowMapPos.w;
    
    // Convert the normalized device coordinates to UVs for sampling
    float2 shadowUV = (input.shadowMapPos.xy * 0.5f) + 0.5f; // [-1,1] to [0,1] 
    shadowUV.y = 1 - shadowUV.y; // Flip the Y
    
    
    // Calculate this pixel's depth from the light
    float depthFromLight = input.shadowMapPos.z;
    
    // Compares the depth from the light and the value in the shadow map
    float distShadowMap = ShadowMap.Sample(BasicSampler, shadowUV).r;
    
    // Sample the shadow map using a comparison sampler, which
	// will compare the depth from the light and the value in the shadow map
	// Note: This is applied below, after we calc our DIRECTIONAL LIGHT
    float shadowAmount = ShadowMap.SampleCmpLevelZero(ShadowSampler, shadowUV, depthFromLight);
    
    for (int i = 0; i < lightCount; i++)
    {
        Light light = lights[i];
        light.direction = normalize(light.direction); // away from light source
        
        switch (light.type)
        {
            case LIGHT_TYPE_DIRECTIONAL:
                float3 dirLightResult = DirLightPBR(light, input.normal, input.worldPosition, cameraPosition, roughness, metal, surfaceColor.rgb, specularColor);
                pixelColor += dirLightResult * (light.castsShadows ? shadowAmount : 1.0f);
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

