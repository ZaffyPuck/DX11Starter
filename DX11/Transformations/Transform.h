#pragma once
#include <DirectXMath.h>

class Transform
{
private:
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 rotation; //pitchYawRoll
	DirectX::XMFLOAT3 scale;

	DirectX::XMFLOAT4X4 worldMatrix;
	DirectX::XMFLOAT4X4 worldMatrixInverseTransposed;
public:
	Transform();
	~Transform();
	void UpdateMatrix();

	// Getters
	DirectX::XMFLOAT3 GetPosition();
	DirectX::XMFLOAT3 GetRotation(); // XMFLOAT4 GetRotation() for quaternion
	DirectX::XMFLOAT3 GetScale();
	DirectX::XMFLOAT4X4 GetWorldMatrix();
	DirectX::XMFLOAT4X4 GetWorldInverseTransposeMatrix();

	// Setters
	void SetPosition(float x, float y, float z);
	void SetPosition(DirectX::XMFLOAT3 position);
	void SetRotation(float pitch, float yaw, float roll);
	void SetRotation(DirectX::XMFLOAT3 rotation); // XMFLOAT4 for quaternion
	void SetScale(float x, float y, float z);
	void SetScale(float uniformScalar);
	void SetScale(DirectX::XMFLOAT3 scale);

	// Transformers
	void MoveAbsolute(float x, float y, float z);
	void MoveAbsolute(DirectX::XMFLOAT3 offset);
	void Rotate(float pitch, float yaw, float roll);
	void Rotate(DirectX::XMFLOAT3 rotation);
	void Scale(float x, float y, float z);
	void Scale(float uniformScalar);
	void Scale(DirectX::XMFLOAT3 scale);
};

