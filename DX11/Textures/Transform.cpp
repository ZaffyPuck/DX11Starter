#include "Transform.h"

Transform::Transform() :
	position(0, 0, 0),
	rotation(0, 0, 0),
	scale(1, 1, 1),
	up(0, 1, 0),
	right(1, 0, 0),
	forward(0, 0, 1),
	areVectorsUTD(true),
	areMatricesUTD(true)
{
	// Init matrices
	XMStoreFloat4x4(&worldMatrix, DirectX::XMMatrixIdentity()); // XM = DirectX Matrix
	XMStoreFloat4x4(&worldMatrixInverseTransposed, DirectX::XMMatrixIdentity());
}
Transform::~Transform(){}
void Transform::UpdateVectors()
{
	// Get
	DirectX::XMVECTOR currentVRotation = DirectX::XMLoadFloat3(&rotation);
	// Make
	DirectX::XMVECTOR currentQRotation = DirectX::XMQuaternionRotationRollPitchYawFromVector(currentVRotation);
	DirectX::XMVECTOR qForward = DirectX::XMVectorSet(0, 0, 1, 0);
	DirectX::XMVECTOR qRight = DirectX::XMVectorSet(1, 0, 0, 0);
	DirectX::XMVECTOR qUp = DirectX::XMVectorSet(0, 1, 0, 0);
	// Set // what is this doing? local to world space?
	DirectX::XMVECTOR cForward = DirectX::XMVector3Rotate(qForward, currentQRotation);
	DirectX::XMVECTOR cRight = DirectX::XMVector3Rotate(qRight, currentQRotation);
	DirectX::XMVECTOR cUp = DirectX::XMVector3Rotate(qUp, currentQRotation);
	// Store
	DirectX::XMStoreFloat3(&forward, cForward);
	DirectX::XMStoreFloat3(&right, cRight);
	DirectX::XMStoreFloat3(&up, cUp);
}
void Transform::UpdateMatrices()
{
	// Get
	DirectX::XMVECTOR currentPosition = DirectX::XMLoadFloat3(&position);
	DirectX::XMVECTOR currentRotation = DirectX::XMLoadFloat3(&rotation);
	DirectX::XMVECTOR currentScale = DirectX::XMLoadFloat3(&scale);
	// Set
	DirectX::XMMATRIX translation = DirectX::XMMatrixTranslationFromVector(currentPosition);		//DirectX::XMMatrixTranslation();
	DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationRollPitchYawFromVector(currentRotation);	// DirectX::XMMatrixRotationRollPitchYaw();
	DirectX::XMMATRIX scale = DirectX::XMMatrixScalingFromVector(currentScale);						// DirectX::XMMatrixScaling();
	// Combine/Calculate
	DirectX::XMMATRIX world = scale * rotation * translation; // XMMatrixMultiply(XMMatrixMultiply(s, r), t))
	DirectX::XMMATRIX worldInverse = DirectX::XMMatrixInverse(0, XMMatrixTranspose(world));
	// Store
	DirectX::XMStoreFloat4x4(&worldMatrix, world);
	DirectX::XMStoreFloat4x4(&worldMatrixInverseTransposed, worldInverse);
}

// Getters
DirectX::XMFLOAT3 Transform::GetPosition(){ return position; }
DirectX::XMFLOAT3 Transform::GetRotation(){ return rotation; }
DirectX::XMFLOAT3 Transform::GetScale(){ return scale; }
//	Local Vectors
DirectX::XMFLOAT3 Transform::GetForward()
{
	UpdateVectors();
	return forward;
}
DirectX::XMFLOAT3 Transform::GetRight()
{
	UpdateVectors();
	return right;
}
DirectX::XMFLOAT3 Transform::GetUp()
{
	UpdateVectors();
	return up;
}
DirectX::XMFLOAT4X4 Transform::GetWorldMatrix()
{
	UpdateMatrices();
	return worldMatrix;
}
DirectX::XMFLOAT4X4 Transform::GetWorldInverseTransposeMatrix()
{
	UpdateMatrices();
	return worldMatrixInverseTransposed;
}

// Setters
void Transform::SetPosition(float x, float y, float z)
{
	position.x = x;
	position.y = y;
	position.z = z;
}
void Transform::SetPosition(DirectX::XMFLOAT3 position)
{
	this->position = position;
}
void Transform::SetRotation(float pitch, float yaw, float roll)
{
	rotation.x = pitch;
	rotation.y = yaw;
	rotation.z = roll;
}
void Transform::SetRotation(DirectX::XMFLOAT3 rotation)
{
	this->rotation = rotation;
}
void Transform::SetScale(float x, float y, float z)
{
	scale.x = x;
	scale.y = y;
	scale.z = z;
}
void Transform::SetScale(float uniformScalar)
{
	scale.x = uniformScalar;
	scale.y = uniformScalar;
	scale.z = uniformScalar;
}
void Transform::SetScale(DirectX::XMFLOAT3 scale)
{
	this->scale.x = scale.x;
	this->scale.y = scale.y;
	this->scale.z = scale.z;
}

// Transformers
// absolue = world?
void Transform::MoveRelative(float x, float y, float z)
{
	DirectX::XMVECTOR reqMovement = DirectX::XMVectorSet(x, y, z, 0);
	DirectX::XMVECTOR currentVRotation = DirectX::XMLoadFloat3(&rotation);
	DirectX::XMVECTOR currentQRotation = DirectX::XMQuaternionRotationRollPitchYawFromVector(currentVRotation); // RotationQuaternion
	DirectX::XMVECTOR resMovement = DirectX::XMVector3Rotate(reqMovement, currentQRotation);
	DirectX::XMVECTOR currentPosition = DirectX::XMLoadFloat3(&position);
	DirectX::XMVECTOR finMovement = DirectX::XMVectorAdd(currentPosition, resMovement);
	DirectX::XMStoreFloat3(&position, finMovement); // Not working for some reason
}
void Transform::MoveRelative(DirectX::XMFLOAT3 offset)
{
	MoveRelative(offset.x, offset.y, offset.z);
}
void Transform::MoveAbsolute(float x, float y, float z)
{
	position.x += x;
	position.y += y;
	position.z += z;
}
void Transform::MoveAbsolute(DirectX::XMFLOAT3 offset)
{
	position.x += offset.x;
	position.y += offset.y;
	position.z += offset.z;
}
void Transform::Rotate(float pitch, float yaw, float roll)
{
	rotation.x += pitch;
	rotation.y += yaw;
	rotation.z += roll;
}
void Transform::Rotate(DirectX::XMFLOAT3 rotation)
{
	this->rotation.x += rotation.x;
	this->rotation.y += rotation.y;
	this->rotation.z += rotation.z;
}
void Transform::Scale(float x, float y, float z)
{
	scale.x *= x;
	scale.y *= y;
	scale.z *= z;
}
void Transform::Scale(float uniformScalar)
{
	scale.x *= uniformScalar;
	scale.y *= uniformScalar;
	scale.z *= uniformScalar;
}
void Transform::Scale(DirectX::XMFLOAT3 scalar)
{
	this->scale.x *= scale.x;
	this->scale.y *= scale.y;
	this->scale.z *= scale.z;
}
