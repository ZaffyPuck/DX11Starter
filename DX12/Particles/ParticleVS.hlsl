cbuffer ExternalData : register(b0)
{
    matrix world;
    matrix worldInverseTranspose;
    matrix view;
    float currentTime;
};

struct Particle
{
    float EmitTime;
    float3 InitVelocity;

    float3 InitPosition;
    float InitRotationAngle;

    float EndRotationAngle;
    float3 padding;
};

// Buffer of particle data
StructuredBuffer<Particle> ParticleData : register(t0);

float4 main( float4 pos : POSITION ) : SV_POSITION
{
    // Grab one particle and calculate its age
    Particle p = ParticleData.Load(particleID);
    float age = currentTime - p.EmitTime; // currentTime is from C++

	return pos;
}