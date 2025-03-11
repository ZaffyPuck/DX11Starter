#include "HeaderStructs.hlsli"
#define LIGHT_AMOUNT 7
float Diffuse(float3 normal, float3 dirToLight)
{
    float dotProd = dot(normal, dirToLight);
    return saturate(dotProd); // disallows negatives (clamps to [0,1])
}
float Attenuate(Light light, float3 worldPos)
{
    float dist = distance(light.position, worldPos);
    float att = saturate(1.0f - (dist * dist / (light.range * light.range)));
    return att * att;
}
float SpecularPhong(Light light, float3 normal, float roughness, float3 dirToCamera)
{
    float3 reflection = reflect(light.direction, normal);
    float specExponent = (1 - roughness) * MAX_SPECULAR_EXPONENT;
    return pow(saturate(dot(reflection, dirToCamera)), specExponent);
}

// Constant buffer
cbuffer ExternalData : register(b0)
{
    float3 colorTint;
    float roughness;
    float3 ambientColor;
    float3 cameraPosition;
    Light lights[LIGHT_AMOUNT];
}

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
    float3 pixelColor = ambientColor * colorTint;
    
    for (int i = 0; i < LIGHT_AMOUNT; i++)
    {
        Light light = lights[i];
        float3 dirToLightSource;
        light.direction = normalize(light.direction); // away from light source
        float3 dirToCamera = normalize(cameraPosition - input.worldPosition); // pixel world position
        float diffusion;
        float specular;
        float3 finalColor;
        float attenuation;
        
        switch (light.type)
        {
            case LIGHT_TYPE_DIRECTIONAL:
                dirToLightSource = -light.direction;
        
                diffusion = Diffuse(input.normal, dirToLightSource);
                specular = SpecularPhong(light, input.normal, roughness, dirToCamera);
            
                finalColor = colorTint * diffusion + specular; // Don’t tint specular? // //float3 light = colorTint * (diffuse + specular); // Tint specular?
                pixelColor += finalColor * light.intensity * light.color;
                break;
            case LIGHT_TYPE_POINT:
                dirToLightSource = normalize(light.position - input.worldPosition);
            
                attenuation = Attenuate(light, input.worldPosition);
                diffusion = Diffuse(input.normal, dirToLightSource);
                specular = SpecularPhong(light, input.normal, roughness, dirToCamera);
            
                finalColor = colorTint * diffusion + specular;
                pixelColor += finalColor * attenuation * light.intensity * light.color;
                break;
            case LIGHT_TYPE_SPOT:
                dirToLightSource = normalize(light.position - input.worldPosition);
                float penumbra = pow(saturate(dot(-dirToLightSource, light.direction)), light.spotFalloff);

                attenuation = Attenuate(light, input.worldPosition);
                diffusion = Diffuse(input.normal, dirToLightSource);
                specular = SpecularPhong(light, input.normal, roughness, dirToCamera);
            
                finalColor = colorTint * diffusion + specular;
                pixelColor += finalColor * attenuation * light.intensity * light.color * penumbra;
                break;
        }
    }
    
    return float4(pixelColor, 1);
}

