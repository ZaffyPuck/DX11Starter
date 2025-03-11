#pragma once
#include <DirectXMath.h>

class Transform
{
private:
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 rotation; //pitchYawRoll
	DirectX::XMFLOAT3 scale;
	// Local Vectors
	DirectX::XMFLOAT3 forward;
	DirectX::XMFLOAT3 right;
	DirectX::XMFLOAT3 up;

	DirectX::XMFLOAT4X4 worldMatrix;
	DirectX::XMFLOAT4X4 worldMatrixInverseTransposed;

	// TODO: Cleaners
	bool areVectorsUTD; // UTD=Up-to-date
	bool areMatricesUTD;
public:
	Transform();
	~Transform();
	void UpdateVectors();
	void UpdateMatrices();

	// - Getters
	DirectX::XMFLOAT3 GetPosition();
	DirectX::XMFLOAT3 GetRotation(); // XMFLOAT4 GetRotation() for quaternion
	DirectX::XMFLOAT3 GetScale();
	// -- Local
	DirectX::XMFLOAT3 GetForward();
	DirectX::XMFLOAT3 GetRight();
	DirectX::XMFLOAT3 GetUp();
	// -- Matrices
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

	// Transform
	void MoveRelative(float x, float y, float z);
	void MoveRelative(DirectX::XMFLOAT3 offset);
	void MoveAbsolute(float x, float y, float z);
	void MoveAbsolute(DirectX::XMFLOAT3 offset);
	void Rotate(float pitch, float yaw, float roll);
	void Rotate(DirectX::XMFLOAT3 rotation);
	void Scale(float x, float y, float z);
	void Scale(float uniformScalar);
	void Scale(DirectX::XMFLOAT3 scale);
};

