#include "Emitter.h"

// Helper macro for getting a float between min and max
#ifndef RandomRange
#define RandomRange(min, max) (float)rand() / RAND_MAX * (max - min) + min
#endif

Emitter::Emitter(
	Microsoft::WRL::ComPtr<ID3D11Device> device,
	std::shared_ptr<Material> material,
	int particleCapacity,
	float particleLifetime,
	int particlesPerSecond,
	float startSize,
	float endSize,
	bool constrainYAxis,
	DirectX::XMFLOAT4 startColor,
	DirectX::XMFLOAT4 endColor,
	DirectX::XMFLOAT3 emitterPosition,
	DirectX::XMFLOAT3 positionRandomRange,
	DirectX::XMFLOAT2 rotationStartMinMax,
	DirectX::XMFLOAT2 rotationEndMinMax,
	DirectX::XMFLOAT3 startVelocity,
	DirectX::XMFLOAT3 velocityRandomRange,
	DirectX::XMFLOAT3 emitterAcceleration,
	unsigned int spriteSheetWidth,
	unsigned int spriteSheetHeight,
	float spriteSheetSpeedScale)
	:
	device(device),
	material(material),
	particleCapacity(particleCapacity),
	particleLifetime(particleLifetime),
	particlesPerSecond(particlesPerSecond),
	secondsPerParticle((float)1 / particlesPerSecond),
	startSize(startSize),
	endSize(endSize),
	constrainYAxis(constrainYAxis),
	startColor(startColor),
	endColor(endColor),
	positionRandomRange(positionRandomRange),
	rotationStartMinMax(rotationStartMinMax),
	rotationEndMinMax(rotationEndMinMax),
	startVelocity(startVelocity),
	velocityRandomRange(velocityRandomRange),
	emitterAcceleration(emitterAcceleration),
	spriteSheetWidth(max(spriteSheetWidth, 1)),
	spriteSheetHeight(max(spriteSheetHeight, 1)),
	spriteSheetFrameWidth(1.0f / spriteSheetWidth),
	spriteSheetFrameHeight(1.0f / spriteSheetHeight),
	spriteSheetSpeedScale(spriteSheetSpeedScale),
	particles(0),
	timeSinceLastEmit(0.f),
	firstLiveParticleIndex(0),
	firstDeadParticleIndex(0),
	liveParticleCount(0)
{
	this->transform.SetPosition(emitterPosition);

	// Delete and release existing resources
	if (particles) delete[] particles;
	indexBuffer.Reset();
	particleDataBuffer.Reset();
	particleDataSRV.Reset();

	// Set up the particle array
	particles = new Particle[particleCapacity];
	ZeroMemory(particles, sizeof(Particle) * particleCapacity);

	// Create an index buffer for particle drawing
	// indices as if we had two triangles per particle
	int numIndices = particleCapacity * 6;
	unsigned int* indices = new unsigned int[numIndices];
	int indexCount = 0;
	for (int i = 0; i < particleCapacity * 4; i += 4)
	{
		indices[indexCount++] = i;
		indices[indexCount++] = i + 1;
		indices[indexCount++] = i + 2;
		indices[indexCount++] = i;
		indices[indexCount++] = i + 2;
		indices[indexCount++] = i + 3;
	}
	D3D11_SUBRESOURCE_DATA indexData = {};
	indexData.pSysMem = indices;

	// Regular (static) index buffer
	D3D11_BUFFER_DESC ibDesc = {};
	ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibDesc.CPUAccessFlags = 0;
	ibDesc.Usage = D3D11_USAGE_DEFAULT;
	ibDesc.ByteWidth = sizeof(unsigned int) * particleCapacity * 6; // 3 for 2 triangles to make a quad
	device->CreateBuffer(&ibDesc, &indexData, indexBuffer.GetAddressOf());
	delete[] indices; // Sent to GPU already

	// Make a dynamic buffer to hold all particle data on GPU
	// Note: We'll be overwriting this every frame with new lifetime data
	D3D11_BUFFER_DESC desc = {};
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	desc.StructureByteStride = sizeof(Particle);
	desc.ByteWidth = sizeof(Particle) * particleCapacity;
	device->CreateBuffer(&desc, 0, particleDataBuffer.GetAddressOf());
	// Create an SRV that points to a structured buffer of particles so we can grab this data in a vertex shader
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = particleCapacity;
	device->CreateShaderResourceView(particleDataBuffer.Get(), &srvDesc, particleDataSRV.GetAddressOf());
}

Emitter::~Emitter()
{
	delete[] particles;
}

void Emitter::UpdateParticle(int particleIndex, float currentTime)
{
	if (IsParticleDone(particleIndex, currentTime))
	{
		FinishParticle(); // no need to pass index since we overwrite it
	}
}
bool Emitter::IsParticleDone(int particleIndex, float currentTime)
{
	float elapsedTimeAlive = currentTime - particles[particleIndex].StartEmittingTimestamp;
	return elapsedTimeAlive >= particleLifetime;
}
void Emitter::FinishParticle()
{
	firstLiveParticleIndex++;
	firstLiveParticleIndex %= particleCapacity;
	liveParticleCount--;
}

void Emitter::Update(float dt, float currentTime)
{ // track lifetimes and emit particles
	if (liveParticleCount > 0)
	{
		// Particles are in one chunk (no wrap)
		if (firstLiveParticleIndex < firstDeadParticleIndex)
		{
			for (int i = firstLiveParticleIndex; i < firstDeadParticleIndex; i++)
			{
				UpdateParticle(i, currentTime);
			}
		}
		// Particles are in two chunks (wraps)
		else if (firstDeadParticleIndex < firstLiveParticleIndex)
		{ // Update young to old
			// firstLive to max
			for (int i = firstLiveParticleIndex; i < particleCapacity; i++)
			{
				UpdateParticle(i, currentTime);
			}

			// min (0) to first dead
			for (int i = 0; i < firstDeadParticleIndex; i++)
			{
				UpdateParticle(i, currentTime);
			}
		}
		else
		{
			// First alive is EQUAL TO first dead, so they're either all alive or all dead
			// - Since we know at least one is alive, they should all be
			for (int i = 0; i < particleCapacity; i++)
			{
				UpdateParticle(i, currentTime);
			}
		}
	}

	// Add to the time
	timeSinceLastEmit += dt;

	// Enough time to emit?
	while (timeSinceLastEmit > secondsPerParticle)
	{
		EmitParticle(currentTime);
		timeSinceLastEmit -= secondsPerParticle;
	}
}
void Emitter::EmitParticle(float currentTime)
{
	// Any left to spawn?
	if (liveParticleCount == particleCapacity)
		return;

	// Which particle is spawning?
	int spawnedIndex = firstDeadParticleIndex;

	// Update the spawn time
	particles[spawnedIndex].StartEmittingTimestamp = currentTime;

	// Adjust the particle start position based on the random range (box shape)
	particles[spawnedIndex].StartPosition = transform.GetPosition();
	particles[spawnedIndex].StartPosition.x += positionRandomRange.x * RandomRange(-1.0f, 1.0f);
	particles[spawnedIndex].StartPosition.y += positionRandomRange.y * RandomRange(-1.0f, 1.0f);
	particles[spawnedIndex].StartPosition.z += positionRandomRange.z * RandomRange(-1.0f, 1.0f);

	// Adjust particle start velocity based on random range
	particles[spawnedIndex].StartVelocity = startVelocity;
	// Forces particles in the -1x and -1y direction????? velocityRandomRange seems to be changing but not visibly
	//particles[spawnedIndex].StartVelocity.x += velocityRandomRange.x * RandomRange(-1.0f, 1.0f);
	//particles[spawnedIndex].StartVelocity.y += velocityRandomRange.y * RandomRange(-1.0f, 1.0f);
	//particles[spawnedIndex].StartVelocity.z += velocityRandomRange.z * RandomRange(-1.0f, 1.0f);

	// Adjust start and end rotation values based on range
	particles[spawnedIndex].StartRotationAngle = RandomRange(rotationStartMinMax.x, rotationStartMinMax.y);
	particles[spawnedIndex].EndRotationAngle = RandomRange(rotationEndMinMax.x, rotationEndMinMax.y);

	// Increment the first dead particle (since it's now alive)
	firstDeadParticleIndex++;
	firstDeadParticleIndex %= particleCapacity; // Wrap

	// One more living particle
	liveParticleCount ++;
}

void Emitter::Draw(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, std::shared_ptr<Camera> camera, float currentTime)
{ // perform any CPU->GPU copies and draw this emitter
		// Now that we have emit and updated all particles for this frame,
	// we can copy them to the GPU as either one big chunk or two smaller chunks

	// Map the buffer
	D3D11_MAPPED_SUBRESOURCE mapped = {};
	context->Map(particleDataBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);

	// How are living particles arranged in the buffer?
	if (firstLiveParticleIndex < firstDeadParticleIndex)
	{
		// Only copy from FirstAlive -> FirstDead
		memcpy(
			mapped.pData, // Destination = start of particle buffer
			particles + firstLiveParticleIndex, // Source = particle array, offset to first living particle
			sizeof(Particle) * liveParticleCount); // Amount = number of particles (measured in BYTES!)
	}
	else
	{
		// Copy from 0 -> FirstDead
		memcpy(
			mapped.pData, // Destination = start of particle buffer
			particles, // Source = start of particle array
			sizeof(Particle) * firstDeadParticleIndex); // Amount = particles up to first dead (measured in BYTES!)

		// ALSO copy from FirstAlive -> End
		memcpy(
			(void*)((Particle*)mapped.pData + firstDeadParticleIndex), // Destination = particle buffer, AFTER the data we copied in previous memcpy()
			particles + firstLiveParticleIndex,  // Source = particle array, offset to first living particle
			sizeof(Particle) * (particleCapacity - firstLiveParticleIndex)); // Amount = number of living particles at end of array (measured in BYTES!)
	}

	// Unmap now that we're done copying
	context->Unmap(particleDataBuffer.Get(), 0);

	// Set up buffers - note that we're NOT using a vertex buffer!
	// When we draw, we'll calculate the number of vertices we expect
	// to have given how many particles are currently alive.  We'll
	// construct the actual vertex data on the fly in the shader.
	UINT stride = 0;
	UINT offset = 0;
	ID3D11Buffer* nullBuffer = 0;
	context->IASetVertexBuffers(0, 1, &nullBuffer, &stride, &offset);
	context->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

	// Set particle-specific data and let the
	// material take care of the rest
	material->PrepareMaterial(&transform, camera);

	// Vertex data
	std::shared_ptr<SimpleVertexShader> vs = material->GetVertexShader();
	vs->SetMatrix4x4("view", camera->GetView());
	vs->SetMatrix4x4("projection", camera->GetProjection());

	vs->SetFloat4("startColor", startColor);
	vs->SetFloat4("endColor", endColor);

	vs->SetFloat("currentTime", currentTime);
	vs->SetFloat3("acceleration", emitterAcceleration);

	vs->SetInt("spriteSheetWidth", spriteSheetWidth);
	vs->SetInt("spriteSheetHeight", spriteSheetHeight);
	vs->SetFloat("spriteSheetFrameWidth", spriteSheetFrameWidth);
	vs->SetFloat("spriteSheetFrameHeight", spriteSheetFrameHeight);

	vs->SetFloat("spriteSheetSpeedScale", spriteSheetSpeedScale);
	vs->SetFloat("startSize", startSize);
	vs->SetFloat("endSize", endSize);
	vs->SetFloat("particleLifetime", particleLifetime);

	vs->SetInt("constrainYAxis", constrainYAxis);

	vs->CopyAllBufferData();

	vs->SetShaderResourceView("ParticleData", particleDataSRV);

	// Now that all of our data is in the beginning of the particle buffer,
	// we can simply draw the correct amount of living particle indices.
	// Each particle = 4 vertices = 6 indices for a quad
	context->DrawIndexed(liveParticleCount * 6, 0, 0);
}
