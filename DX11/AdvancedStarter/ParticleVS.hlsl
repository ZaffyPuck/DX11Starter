cbuffer externalData : register(b0)
{
	matrix view;
	matrix projection;

    float4 startColor;
    float4 endColor;

    float currentTime;
    float3 acceleration;

    int spriteSheetWidth;
    int spriteSheetHeight;
    float spriteSheetFrameWidth;
    float spriteSheetFrameHeight;

    float spriteSheetSpeedScale;
    float startSize;
    float endSize;
    float particleLifetime;

    int constrainYAxis;
};

struct Particle
{
    float StartEmittingTimestamp;
    float3 StartPosition;

    float3 StartVelocity;
    float StartRotationAngle;

    float EndRotationAngle;
    float3 padding;
};

struct VertexToPixel
{
	float4 screenPosition	: SV_POSITION;
	float2 uv				: TEXCOORD;
    float4 colorTint        : COLOR;
};

// Buffer of particle data
StructuredBuffer<Particle> ParticleData : register(t0);

// --------------------------------------------------------
// The entry point (main method) for our vertex shader
// --------------------------------------------------------
VertexToPixel main(uint id : SV_VertexID)
{
	// Set up output
    VertexToPixel output;

	// Get id info
    uint particleID = id / 4; // Every group of 4 verts are ONE particle!
    Particle p = ParticleData.Load(particleID);

    float elapsedTimeAlive = currentTime - p.StartEmittingTimestamp;
    float lifetimePercent = elapsedTimeAlive / particleLifetime; // from 0 to 1

	// p = x0 + vt + 0.5 *a * t^2
    float3 pos = p.StartVelocity * elapsedTimeAlive + p.StartPosition + 0.5 * acceleration * elapsedTimeAlive * elapsedTimeAlive;
    //float3 pos = p.InitVelocity * elapsedTimeAlive + p.InitPosition; // no accel

	// Size
    float size = lerp(startSize, endSize, lifetimePercent);

	// Offsets for the 4 corners of a quad - we'll only use one for each vertex, but which one depends on the cornerID
    float2 offsets[4];
    offsets[0] = float2(-1.0f, +1.0f); // TL
    offsets[1] = float2(+1.0f, +1.0f); // TR
    offsets[2] = float2(+1.0f, -1.0f); // BR
    offsets[3] = float2(-1.0f, -1.0f); // BL

	// Handle rotation - get sin/cos and build a rotation matrix
    float s, c, rotation = lerp(p.StartRotationAngle, p.EndRotationAngle, lifetimePercent);
    sincos(rotation, s, c); // One function to calc both sin and cos
    float2x2 rot =
    {
        c, s,
		-s, c
    };

	// Rotate the offset for this corner and apply size
    uint cornerID = id % 4; // 0,1,2,3 = the corner of the particle "quad"
    float2 rotatedOffset = mul(offsets[cornerID], rot) * size;

	// Billboarding!
	// Offset the position based on the camera's right and up vectors
    pos += float3(view._11, view._12, view._13) * rotatedOffset.x; // RIGHT
    pos += (constrainYAxis ? float3(0, 1, 0) : float3(view._21, view._22, view._23)) * rotatedOffset.y; // UP

	// Calculate output position
    matrix viewProj = mul(projection, view);
    output.screenPosition = mul(viewProj, float4(pos, 1.0f));

	// Sprite sheet animation calculations
	// Note: Probably even better to swap shaders here (ParticleVS or AnimatedParticleVS)
	//  but this should work for the demo, as we can think of a non-animated particle
	//  as having a sprite sheet with exactly one frame
    float animPercent = fmod(lifetimePercent * spriteSheetSpeedScale, 1.0f);
    uint ssIndex = (uint) floor(animPercent * (spriteSheetWidth * spriteSheetHeight));

	// Get the U/V indices (basically column & row index across the sprite sheet)
    uint uIndex = ssIndex % spriteSheetWidth;
    uint vIndex = ssIndex / spriteSheetHeight; // Integer division is important here!

	// Convert to a top-left corner in uv space (0-1)
    float u = uIndex / (float) spriteSheetWidth;
    float v = vIndex / (float) spriteSheetHeight;

    float2 uvs[4];
	// Top Left
    uvs[0] = float2(u, v);
	// Top Right
    uvs[1] = float2(u + spriteSheetFrameWidth, v);
	// Bottom Right
    uvs[2] = float2(u + spriteSheetFrameWidth, v + spriteSheetFrameHeight);
	// Bottom Left
    uvs[3] = float2(u, v + spriteSheetFrameHeight);

	// Finalize output
    output.uv = saturate(uvs[cornerID]);
    output.colorTint = lerp(startColor, endColor, lifetimePercent);

    return output;
}