#include "Transform.h"

Transform::Transform() :
	position(0, 0, 0),
	rotation(0, 0, 0),
	scale(1, 1, 1)
{
	// Init matrices
	XMStoreFloat4x4(&worldMatrix, DirectX::XMMatrixIdentity()); // XM = DirectX Matrix
	XMStoreFloat4x4(&worldMatrixInverseTransposed, DirectX::XMMatrixIdentity());
}
Transform::~Transform()
{
}

void Transform::UpdateMatrix()
{
	DirectX::XMMATRIX translation = DirectX::XMMatrixTranslationFromVector(DirectX::XMLoadFloat3(&position)); //DirectX::XMMatrixTranslation();
	DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationRollPitchYawFromVector(DirectX::XMLoadFloat3(&this->rotation)); // DirectX::XMMatrixRotationRollPitchYaw();
	DirectX::XMMATRIX scale = DirectX::XMMatrixScalingFromVector(DirectX::XMLoadFloat3(&this->scale)); // DirectX::XMMatrixScaling();

	DirectX::XMMATRIX world = scale * rotation * translation; // XMMatrixMultiply(XMMatrixMultiply(s, r), t))
	DirectX::XMMATRIX worldInverse = DirectX::XMMatrixInverse(0, XMMatrixTranspose(world));

	DirectX::XMStoreFloat4x4(&worldMatrix, world);
	DirectX::XMStoreFloat4x4(&worldMatrixInverseTransposed, worldInverse);
}

// Getters
DirectX::XMFLOAT3 Transform::GetPosition()
{
	return position;
}
DirectX::XMFLOAT3 Transform::GetRotation()
{
	return rotation;
}
DirectX::XMFLOAT3 Transform::GetScale()
{
	return scale;
}
DirectX::XMFLOAT4X4 Transform::GetWorldMatrix()
{
	UpdateMatrix();
	return worldMatrix;
}
DirectX::XMFLOAT4X4 Transform::GetWorldInverseTransposeMatrix()
{
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
	this->scale.x *= scale.x;
	this->scale.y *= scale.y;
	this->scale.z *= scale.z;
}

// Transformers
// absolue = world?
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
