#pragma once
#include <DirectXMath.h>

#include "Transform.h"

enum class CameraProjectionType
{
	Perspective,
	Orthographic
};

class Camera
{
private:
	CameraProjectionType projectionType;
	Transform transform;
	float aspectRatio;
	float fieldOfView;
	float nearClip;
	float farClip;
	float orthographicWidth;
	// Controls
	float movementSpeed;
	float mouseLookSpeed;
	// Matrices
	DirectX::XMFLOAT4X4 viewMatrix;
	DirectX::XMFLOAT4X4 projMatrix;
public:
	Camera(
		DirectX::XMFLOAT3 position,
		float moveSpeed,
		float mouseLookSpeed,
		float aspectRatio,
		float fieldOfView = 45.f,
		float nearClip = 0.01f,
		float farClip = 200.0f,
		CameraProjectionType projType = CameraProjectionType::Perspective);

	Camera(
		float x, float y, float z,
		float moveSpeed,
		float mouseLookSpeed,
		float aspectRatio,
		float fieldOfView = 45.f,
		float nearClip = 0.01f,
		float farClip = 100.0f,
		CameraProjectionType projType = CameraProjectionType::Perspective);

	~Camera();

	// Updating methods
	void Update(float dt);
	void UpdateViewMatrix();
	void UpdateProjectionMatrix(float aspectRatio);

	// Getters
	CameraProjectionType GetProjectionType();
	Transform* GetTransform();
	float GetFieldOfView();
	float GetAspectRatio();
	float GetNearClip();
	float GetFarClip();
	float GetOrthographicWidth();
	// - Controls
	float GetMovementSpeed();
	float GetMouseLookSpeed();
	// - Matrices
	DirectX::XMFLOAT4X4 GetView();
	DirectX::XMFLOAT4X4 GetProjection();

	// Setters
	void SetProjectionType(CameraProjectionType type);
	void SetFieldOfView(float fov);
	void SetNearClip(float distance);
	void SetFarClip(float distance);
	void SetOrthographicWidth(float width);
	// - Controls
	void SetMovementSpeed(float speed);
	void SetMouseLookSpeed(float speed);
};

