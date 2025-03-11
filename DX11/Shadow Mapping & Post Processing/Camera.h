#pragma once
#include "Transform.h"
#include <DirectXMath.h>

enum Mode
{
	PERSPECTIVE,
	ORTHOGRAPHIC
};
class Camera
{
private:
	Mode mode;
	float orthographicWidth;
	Transform transform;
	DirectX::XMFLOAT4X4 viewMatrix;
	DirectX::XMFLOAT4X4 projectionMatrix;
	float aspectRatio;
	float fieldOfView; // Radians
	float nearClip;
	float farClip;
	float movementSpeed;
	float mouseLookSpeed;
public:
	Camera(
		DirectX::XMFLOAT3 position,
		float aspectRatio,
		float movementSpeed,
		float mouseLookSpeed,
		float fieldOfView = 45.0f,
		float nearClip = 0.001f,
		float farClip = 1000.0f,
		Mode mode = Mode::PERSPECTIVE
	);
	Camera(
		float x, float y, float z,
		float aspectRatio,
		float movementSpeed,
		float mouseLookSpeed,
		float fieldOfView = 45.0f,
		float nearClip = 0.001f,
		float farClip = 1000.0f,
		Mode mode = Mode::PERSPECTIVE
	);
	~Camera();
	void Update(float deltaTime);
	void UpdateViewMatrix();
	void UpdateProjectionMatrix(float aspectRatio);

	// - Getters
	Mode GetMode();
	float GetOrthographicWidth();
	Transform* GetTransform();
	DirectX::XMFLOAT4X4 GetView();
	DirectX::XMFLOAT4X4 GetProjection();
	float GetAspectRatio();
	float GetFieldOfView();
	float GetNearClip();
	float GetFarClip();
	float GetMovementSpeed();
	float GetMouseLookSpeed();

	// - Setters
	void SetMode(Mode mode);
	void SetOrthographicWidth(float orthographicWidth);
	void SetAspectRatio(float aspectRatio);
	void SetFieldOfView(float fieldOfView);
	void SetNearClip(float nearDistance);
	void SetFarClip(float farDistance);
	void SetMovementSpeed(float movementSpeed);
	void SetMouseLookSpeed(float mouseLookSpeed);
};

