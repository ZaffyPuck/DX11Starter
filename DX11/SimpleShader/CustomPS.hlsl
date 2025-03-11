// Custom Shader
cbuffer ExternalData : register(b0)
{
    float3 colorTint;
    float time;
}

struct InputData
{
    float4 screenPosition   : SV_POSITION;
    float3 normal           : NORMAL;
    float2 uv               : TEXCOORD;
};

float4 main(InputData input) : SV_TARGET
{
    float BLOCK_WIDTH = 0.01f;
    float3 COLOR1 = float3(0.0, 0.0, 0.3);
    float3 COLOR2 = float3(0.5, 0.0, 0.0);
    float3 wave_color;
	
    float c1 = fmod(input.uv.x, 2.0 * BLOCK_WIDTH);
    c1 = step(BLOCK_WIDTH, c1);
	
    float c2 = fmod(input.uv.y, 2.0 * BLOCK_WIDTH);
    c2 = step(BLOCK_WIDTH, c2);
	
    float3 bg_color = lerp(input.uv.x * COLOR1, input.uv.y * COLOR2, c1 * c2);
	
	
	// To create the waves
    float wave_width = 0.01;
    input.uv = -1.0 + 2.0 * input.uv;
    input.uv.y += 0.1;
    for (float i = 0.0; i < 10.0; i++)
    {
		
        input.uv.y += (0.07 * sin(input.uv.x + i / 7.0 + time));
        wave_width = abs(1.0 / (150.0 * input.uv.y));
        wave_color += float3(wave_width * 1.9, wave_width, wave_width * 1.5);
    }
	
    float3 final_color = bg_color + wave_color;
	
    return float4(final_color, 1.0f);
    //return float4(colorTint, 1.0f);
}