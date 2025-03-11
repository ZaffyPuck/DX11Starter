#pragma once

#include "Transform.h"
#include "Material.h"

struct Particle // Alligned
{
	float StartEmittingTimestamp; // Timestamp of it starting
	DirectX::XMFLOAT3 StartPosition;

	DirectX::XMFLOAT3 StartVelocity;
	float StartRotationAngle;

	float EndRotationAngle;
	DirectX::XMFLOAT3 padding;
};

// particle manager
class Emitter
{
public:
	Emitter(
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
		unsigned int spriteSheetWidth = 1,
		unsigned int spriteSheetHeight = 1,
		float spriteSheetSpeedScale = 1.f);
	~Emitter();

	void Update(float dt, float currentTime); // track lifetimes and emit particles
	void Draw(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context,
		std::shared_ptr<Camera> camera,
		float currentTime); // perform any CPU->GPU copies and draw this emitter
private:
	// Material & transform
	Transform transform;
	std::shared_ptr<Material> material;

	// Array
	int particleCapacity; // Maximum number of particles
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

	// Visual data
	DirectX::XMFLOAT4 startColor;
	DirectX::XMFLOAT4 endColor;
	float startSize;
	float endSize;
	bool constrainYAxis;

	// Particle randomization ranges
	DirectX::XMFLOAT3 positionRandomRange;
	DirectX::XMFLOAT3 velocityRandomRange;
	DirectX::XMFLOAT2 rotationStartMinMax;
	DirectX::XMFLOAT2 rotationEndMinMax;

	// Emitter-level data (this is the same for all particles)
	DirectX::XMFLOAT3 emitterAcceleration;
	DirectX::XMFLOAT3 startVelocity;

	// Sprite sheet options
	int spriteSheetWidth;
	int spriteSheetHeight;
	float spriteSheetFrameWidth;
	float spriteSheetFrameHeight;
	float spriteSheetSpeedScale;

	//GPU buffer of particles (buffer and SRV), index buffer, texture and shaders
	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<ID3D11Buffer> particleDataBuffer;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> particleDataSRV;
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;

	// Helpers
	void EmitParticle(float currentTime);
	void UpdateParticle(int particleIndex, float currentTime);
	bool IsParticleDone(int particleIndex, float currentTime);
	void FinishParticle();
};

