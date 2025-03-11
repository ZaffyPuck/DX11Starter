// === Defines === //
#define PI 3.141592654f
#define Euler 2.71828182846f

// === Structs === //

// Layout of data in the vertex buffer
struct Vertex
{
    float3 localPosition	: POSITION;
    float2 uv				: TEXCOORD;
    float3 normal			: NORMAL;
    float3 tangent			: TANGENT;
};
static const uint VertexSizeInBytes = 11 * 4; // 11 floats total per vertex * 4 bytes each

// Payload for rays (data that is "sent along" with each ray during raytrace)
// Note: This should be as small as possible
struct RayPayload
{
	float3 color;
    uint recursionDepth;
    uint rayPerPixelIndex;
    // Shadow Ray
    bool isShadowRay;
    bool isShadowRayHit;
};
// Lighting
struct DirectLight
{
    float3 direction;
    float intensity;
    float3 color;
    float padding2;
};

// Note: We'll be using the built-in BuiltInTriangleIntersectionAttributes struct
// for triangle attributes, so no need to define our own.  It contains a single float2.

// === Constant buffers === //

cbuffer SceneData : register(b0)
{
	matrix inverseViewProjection;
	float3 cameraPosition;
    uint maxRecursionDepth;
    uint raysPerPixel;
    float3 padding;
    DirectLight sun;
};


// Ensure this matches C++ buffer struct define!
#define MAX_INSTANCES_PER_BLAS 100
cbuffer ObjectData : register(b1)
{
	float4 entityColor[MAX_INSTANCES_PER_BLAS];
};


// === Resources === //

// Output UAV
RWTexture2D<float4> OutputColor				: register(u0);

// The actual scene we want to trace through (a TLAS)
RaytracingAccelerationStructure SceneTLAS	: register(t0);

// Geometry buffers
ByteAddressBuffer IndexBuffer        		: register(t1);
ByteAddressBuffer VertexBuffer				: register(t2);


// === Helpers === //

// Loads the indices of the specified triangle from the index buffer
uint3 LoadIndices(uint triangleIndex)
{
	// What is the start index of this triangle's indices?
	uint indicesStart = triangleIndex * 3;

	// Adjust by the byte size before loading
	return IndexBuffer.Load3(indicesStart * 4); // 4 bytes per index
}

// Barycentric interpolation of data from the triangle's vertices
Vertex InterpolateVertices(uint triangleIndex, float3 barycentricData)
{
	// Grab the indices
	uint3 indices = LoadIndices(triangleIndex);

	// Set up the final vertex
	Vertex vert;
	vert.localPosition = float3(0, 0, 0);
	vert.uv = float2(0, 0);
	vert.normal = float3(0, 0, 0);
	vert.tangent = float3(0, 0, 0);

	// Loop through the barycentric data and interpolate
	for (uint i = 0; i < 3; i++)
	{
		// Get the index of the first piece of data for this vertex
		uint dataIndex = indices[i] * VertexSizeInBytes;

		// Grab the position and offset
		vert.localPosition += asfloat(VertexBuffer.Load3(dataIndex)) * barycentricData[i];
		dataIndex += 3 * 4; // 3 floats * 4 bytes per float

		// UV
		vert.uv += asfloat(VertexBuffer.Load2(dataIndex)) * barycentricData[i];
		dataIndex += 2 * 4; // 2 floats * 4 bytes per float

		// Normal
		vert.normal += asfloat(VertexBuffer.Load3(dataIndex)) * barycentricData[i];
		dataIndex += 3 * 4; // 3 floats * 4 bytes per float

		// Tangent (no offset at the end, since we start over after looping)
		vert.tangent += asfloat(VertexBuffer.Load3(dataIndex)) * barycentricData[i];
	}

	// Final interpolated vertex data is ready
	return vert;
}

// Calculates an origin and direction from the camera fpr specific pixel indices
void CalcRayFromCamera(float2 rayIndices, out float3 origin, out float3 direction)
{
	// Offset to the middle of the pixel
	float2 pixel = rayIndices + 0.5f;
	float2 screenPos = pixel / DispatchRaysDimensions().xy * 2.0f - 1.0f;
	screenPos.y = -screenPos.y;

	// Unproject the coords
	float4 worldPos = mul(inverseViewProjection, float4(screenPos, 0, 1));
	worldPos.xyz /= worldPos.w;

	// Set up the outputs
	origin = cameraPosition.xyz;
	direction = normalize(worldPos.xyz - origin);
}

// Sampling functions based on Chapter 16 of Raytracing Gems
// Params should be uniform between [0,1]
float3 RandomVector(float u0, float u1)
{
    float a = u0 * 2 - 1;
    float b = sqrt(1 - a * a);
    float phi = 2.0f * PI * u1;

    float x = b * cos(phi);
    float y = b * sin(phi);
    float z = a;

    return float3(x, y, z);
}

// First two params should be uniform between [0,1]
float3 RandomCosineWeightedHemisphere(float u0, float u1, float3 unitNormal)
{
    float a = u0 * 2 - 1;
    float b = sqrt(1 - a * a);
    float phi = 2.0f * PI * u1;

    float x = unitNormal.x + b * cos(phi);
    float y = unitNormal.y + b * sin(phi);
    float z = unitNormal.z + a;

	// float pdf = a / PI;
    return float3(x, y, z);
}

// Based on https://thebookofshaders.com/10/
float rand(float2 uv)
{
    float dotResult = dot(uv, float2(12.9898, 78.233));
    float sinResult = sin(dotResult); // -1 to 1
    return frac(sinResult * 43758.5453); // Gets fraction clamp [0-1]
}

float2 rand2(float2 uv)
{
    float x = rand(uv);
    float y = sqrt(1 - x * x);
    return float2(x, y);
}

float3 rand3(float2 uv)
{
    return float3(
		rand2(uv),
		rand(uv.yx));
}

// Fresnel approximation
float FresnelSchlick(float NdotV, float indexOfRefraction)
{
    float r0 = pow((1.0f - indexOfRefraction) / (1.0f + indexOfRefraction), 2.0f);
    return r0 + (1.0f - r0) * pow(1 - NdotV, 5.0f);
}

// Refraction function that returns a bool depending on result
bool TryRefract(float3 incident, float3 normal, float ior, out float3 refr)
{
    float NdotI = dot(normal, incident);
    float k = 1.0f - ior * ior * (1.0f - NdotI * NdotI);

    if (k < 0.0f)
    {
        refr = float3(0, 0, 0);
        return false;
    }

    refr = ior * incident - (ior * NdotI + sqrt(k)) * normal;
    return true;
}

// interpolation from 0 to 1
float sigmoid(float value, float multiplier)
{
    return 1 / (1 + pow(Euler, -multiplier*value));
}

// === Shaders === //

// Ray generation shader - Launched once for each ray we want to generate (which is generally once per pixel of our output texture)
[shader("raygeneration")]
void RayGen()
{
	// Get the ray indices
	uint2 rayIndices = DispatchRaysIndex().xy;

	// Average color from all rays
    float3 cumulativeColor = float3(0, 0, 0);

    for (int i = 0; i < raysPerPixel; i++)
    {
        float2 pixelPositionOffset = rand((float) i / raysPerPixel);
        float2 offsetIndices = ((float2) rayIndices) + pixelPositionOffset;

		// Calculate the ray data
        float3 rayOrigin;
        float3 rayDirection;
        CalcRayFromCamera(rayIndices, rayOrigin, rayDirection);

		// Set up final ray description
        RayDesc ray;
        ray.Origin = rayOrigin;
        ray.Direction = rayDirection;
        ray.TMin = 0.0001f;
        ray.TMax = 1000.0f;

		// Set up the payload for the ray
        RayPayload payload = (RayPayload) 0; // This initializes the struct to all zeros
        payload.color = float3(1, 1, 1);
        payload.recursionDepth = 0;
        payload.rayPerPixelIndex = i;
        payload.isShadowRay = false;

		// Perform the ray trace for this ray
        TraceRay(
			SceneTLAS,
			RAY_FLAG_NONE,
			0xFF,	// Mask
			0, 0, 0, // Offset (why dont we use this)
			ray,
			payload);

        cumulativeColor += payload.color;
    }
    cumulativeColor /= raysPerPixel;

	// Set the final color of the buffer (gamma corrected)
    float3 gammaCorrectedColor = pow(cumulativeColor, 1.0f / 2.2f);
    OutputColor[rayIndices] = float4(gammaCorrectedColor, 1);
}

// Miss shader - What happens if the ray doesn't hit anything?
[shader("miss")]
void Miss(inout RayPayload payload)
{
    if (payload.isShadowRay)
    {
        payload.isShadowRayHit = false;
        return;
    }

	// Hemispheric gradient
	float3 upColor = float3(0.3f, 0.5f, 0.95f);
	float3 downColor = float3(1, 1, 1);

	// Interpolate based on the direction of the ray
	float interpolation = dot(normalize(WorldRayDirection()), float3(0, 1, 0)) * 0.5f + 0.5f;
	float3 color = lerp(downColor, upColor, interpolation);

	// Alter the payload color by the sky color
    payload.color *= color;
}

// Closest hit shader - Runs when a ray hits the closest surface
[shader("closesthit")]
void ClosestHit(inout RayPayload payload, BuiltInTriangleIntersectionAttributes hitAttributes)
{
	// If we've reached the max recursion, we haven't hit a light source
    if (payload.recursionDepth == maxRecursionDepth)
    {
        payload.color = float3(0, 0, 0);
        return;
    }
    if (payload.isShadowRay)
    {
        return;
    }

        // ----- Pre-Calculations ----- //
    uint instanceID = InstanceID();

	// Adjust the payload color by this instance's color
    payload.color *= entityColor[instanceID].rgb;

	// Calculate the barycentric data for vertex interpolation
    float3 barycentricData = float3(
		1.0f - hitAttributes.barycentrics.x - hitAttributes.barycentrics.y,
		hitAttributes.barycentrics.x,
		hitAttributes.barycentrics.y);

	// Grab the index of the triangle we hit
    uint triangleIndex = PrimitiveIndex();

	// Get the interpolated vertex data (intersection point)
    Vertex interpolatedVertex = InterpolateVertices(triangleIndex, barycentricData);
    float3 worldspaceNormal = normalize(mul(interpolatedVertex.normal, (float3x3) ObjectToWorld4x3()));

    // ----- Shadow Ray ----- //
    float reflDirection = dot(-normalize(sun.direction), worldspaceNormal);
    if (reflDirection > 0)
    {
        RayDesc shadowRay;
        shadowRay.Origin = WorldRayOrigin() + WorldRayDirection() * (RayTCurrent() - 0.01f);
        shadowRay.Direction = -normalize(sun.direction);
        shadowRay.TMin = 0.0001f;
        shadowRay.TMax = 1000.0f;

        // Trace shadow ray
        RayPayload shadowPayload;
        shadowPayload.isShadowRay = true;
        shadowPayload.isShadowRayHit = true;

        TraceRay(
		SceneTLAS,
		RAY_FLAG_NONE,
		0xFF, // Mask
		0, 0, 0, // Offset
		shadowRay,
		shadowPayload);

        if (shadowPayload.isShadowRayHit) // could probably carry this through color, but this is more readable
        {
            payload.color *= saturate(1.2 - sun.intensity); // adds .2 since it would be flat black if intensity is 1
        }
    }
    else
    {
        // Attempted different falloff methods
        //float staticIntensity = saturate(1.2 - sun.intensity);
        //float interpolatedIntensity = 1 - reflDirection; // general interpolation
        float normalizedIntensity = (reflDirection * 2) + 1; // 0 to -1 -> 1 to -1
        float interpolatedIntensity = sigmoid(normalizedIntensity, 5);
        payload.color *= interpolatedIntensity;

    }

    // ----- Calculations ----- //
	// Diffuse vs Specular
	// Calc a unique RNG value for this ray, based on the "uv" of this pixel and other per-ray data
    float2 uv = (float2) DispatchRaysIndex() / (float2) DispatchRaysDimensions();
    float2 rng = rand2(uv * (payload.recursionDepth + 1) + payload.rayPerPixelIndex + RayTCurrent());

	// Interpolate between perfect reflection and random bounce based on roughness
    float3 reflection = reflect(WorldRayDirection(), worldspaceNormal);
    float3 randomBounce = RandomCosineWeightedHemisphere(rand(rng), rand(rng.yx), worldspaceNormal);
    float3 newDirection = normalize(lerp(reflection, randomBounce, saturate(pow(entityColor[instanceID].a,2))));

    // ----- New Ray ----- //
    RayDesc ray;
    ray.Origin = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    ray.Direction = newDirection;
    ray.TMin = 0.0001f;
    ray.TMax = 1000.0f;

    payload.recursionDepth++;

	// Recurse
    TraceRay(
		SceneTLAS,
		RAY_FLAG_NONE,
		0xFF, // Mask
		0, 0, 0, // Offset (why dont we use this)
		ray,
		payload);
}

// Closest hit shader - Runs when a ray hits the closest surface
[shader("closesthit")]
void ClosestHitTransparent(inout RayPayload payload, BuiltInTriangleIntersectionAttributes hitAttributes)
{
	// If we've reached the max recursion, we haven't hit a light source
    if (payload.recursionDepth == maxRecursionDepth)
    {
        payload.color = float3(0, 0, 0);
        return;
    }
    if (payload.isShadowRay)
    {
        return;
    }

	uint instanceID = InstanceID();

	// We've hit, so adjust the payload color by this instance's color
    payload.color *= entityColor[instanceID].rgb;

	// Get the geometry hit details and convert normal to world space
    	// Calculate the barycentric data for vertex interpolation
    float3 barycentricData = float3(
		1.0f - hitAttributes.barycentrics.x - hitAttributes.barycentrics.y,
		hitAttributes.barycentrics.x,
		hitAttributes.barycentrics.y);

	// Grab the index of the triangle we hit
    uint triangleIndex = PrimitiveIndex();

	// Get the interpolated vertex data (intersection point)
    Vertex interpolatedVertex = InterpolateVertices(triangleIndex, barycentricData);

	// Get the data for this entity
    float3 worldspaceNormal = normalize(mul(interpolatedVertex.normal, (float3x3) ObjectToWorld4x3()));

	// Calc a unique RNG value for this ray, based on the "uv" of this pixel and other per-ray data
    float2 uv = (float2) DispatchRaysIndex() / (float2) DispatchRaysDimensions();
    float2 rng = rand2(uv * (payload.recursionDepth + 1) + payload.rayPerPixelIndex + RayTCurrent());

	// Get the index of refraction based on the side of the hit
    float indexOfRefraction = 1.5f;
    if (HitKind() == HIT_KIND_TRIANGLE_FRONT_FACE)
    {
		// Invert the index of refraction for front faces
        indexOfRefraction = 1.0f / indexOfRefraction;
    }
    else
    {
		// Invert the normal for back faces
        worldspaceNormal *= -1;
    }

	// Random chance for reflection instead of refraction based on Fresnel
    float NdotV = dot(-WorldRayDirection(), worldspaceNormal);
    bool reflectFresnel = FresnelSchlick(NdotV, indexOfRefraction) > rand(rng);

	// Test for refraction
    float3 refraction;
    if (reflectFresnel || !TryRefract(WorldRayDirection(), worldspaceNormal, indexOfRefraction, refraction))
    {
        refraction = reflect(WorldRayDirection(), worldspaceNormal);
    }

	// Interpolate between refract/reflect and random bounce based on roughness squared
    float3 randomReflection = RandomCosineWeightedHemisphere(rand(rng), rand(rng.yx), worldspaceNormal);
    float3 direction = normalize(lerp(refraction, randomReflection, saturate(entityColor[instanceID].a)));


	// Create the new recursive ray
    RayDesc ray;
    ray.Origin = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    ray.Direction = direction;
    ray.TMin = 0.0001f;
    ray.TMax = 1000.0f;

	// Recursive ray trace
    payload.recursionDepth++;
    TraceRay(
		SceneTLAS,
		RAY_FLAG_NONE,
		0xFF, 0, 0, 0, // Mask and offsets
		ray,
		payload);
}

// Closest hit shader - Runs when a ray hits the closest surface
[shader("closesthit")]
void ClosestHitEmissive(inout RayPayload payload, BuiltInTriangleIntersectionAttributes hitAttributes)
{
    if (payload.isShadowRay)
    {
        return;
    }

    uint instanceID = InstanceID();
    float4 color = entityColor[instanceID];
    payload.color = color.rgb * color.a; // Apply intensity for emissive material
}

// Closest hit shader - Runs when a ray hits the closest surface
[shader("closesthit")]
void ClosestHitShadow(inout RayPayload payload, BuiltInTriangleIntersectionAttributes hitAttributes)
{
	// If we've reached the max recursion, we haven't hit a light source
    if (payload.recursionDepth == maxRecursionDepth)
    {
        payload.color = float3(0, 0, 0);
        return;
    }
    if (payload.isShadowRay)
    {
        return;
    }

    // Send out next ray
    uint instanceID = InstanceID();

	// Adjust the payload color by this instance's color
    payload.color *= entityColor[instanceID].rgb;

	// Calculate the barycentric data for vertex interpolation
    float3 barycentricData = float3(
		1.0f - hitAttributes.barycentrics.x - hitAttributes.barycentrics.y,
		hitAttributes.barycentrics.x,
		hitAttributes.barycentrics.y);

	// Grab the index of the triangle we hit
    uint triangleIndex = PrimitiveIndex();

	// Get the interpolated vertex data (intersection point)
    Vertex interpolatedVertex = InterpolateVertices(triangleIndex, barycentricData);

	// Get the data for this entity
    float3 worldspaceNormal = normalize(mul(interpolatedVertex.normal, (float3x3) ObjectToWorld4x3()));

    	// Diffuse vs Specular
	// Calc a unique RNG value for this ray, based on the "uv" of this pixel and other per-ray data
    float2 uv = (float2) DispatchRaysIndex() / (float2) DispatchRaysDimensions();
    float2 rng = rand2(uv * (payload.recursionDepth + 1) + payload.rayPerPixelIndex + RayTCurrent());

	// Interpolate between perfect reflection and random bounce based on roughness
    float3 reflection = reflect(WorldRayDirection(), worldspaceNormal);
    float3 randomBounce = RandomCosineWeightedHemisphere(rand(rng), rand(rng.yx), worldspaceNormal);
    float3 direction = normalize(lerp(reflection, randomBounce, saturate(pow(entityColor[instanceID].a, 2))));

    RayDesc ray;
    ray.Origin = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    ray.Direction = direction;
    ray.TMin = 0.0001f;
    ray.TMax = 1000.0f;

    payload.recursionDepth++;

    // Recurse
    TraceRay(
		SceneTLAS,
		RAY_FLAG_NONE,
		0xFF, // Mask
		0, 0, 0, // Offset
		ray,
		payload);
}