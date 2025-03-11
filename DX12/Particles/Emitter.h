#pragma once

#include <stdlib.h>     // shared ptr
#include "Transform.h"
#include "Material.h"

struct Particle // Alligned
{
	float emitTime; // Timestamp of it starting
	DirectX::XMFLOAT3 InitVelocity;

	DirectX::XMFLOAT3 InitPosition;
	float InitRotationAngle;

	float EndRotationAngle;
	DirectX::XMFLOAT3 padding;
};

// particle manager
class Emitter
{
public:
	Emitter();
	~Emitter();
	void Update(); // track lifetimes and emit particles
	void Draw(); // perform any CPU->GPU copies and draw this emitter
private:
	// Material & transform
	Transform transform;
	std::shared_ptr<Material> material;

	// Array
	int maxParticles; // Maximum number of particles
	Particle* particles; // All possible particles

	// Tracking
	int firstDeadParticleIndex;
	int firstLiveParticleIndex;
	int liveParticleCount;

	// Emission Properties
	float particleLifetime;
	int particlesPerSecond;
	float secondsPerParticle;
	float timeSinceLastEmit;

	//GPU buffer of particles (buffer and SRV), index buffer, texture and shaders
};

