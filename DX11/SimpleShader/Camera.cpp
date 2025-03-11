#include "Camera.h"
#include "Input.h"

Camera::Camera(
    DirectX::XMFLOAT3 position,
    float aspectRatio,
    float movementSpeed,
    float mouseLookSpeed,
    float fieldOfView,
    float nearClip,
    float farClip,
    Mode mode) :
    aspectRatio(aspectRatio),
    movementSpeed(movementSpeed),
    mouseLookSpeed(mouseLookSpeed),
    fieldOfView(fieldOfView),
    nearClip(nearClip),
    farClip(farClip),
    mode(mode),
    orthographicWidth(5.0f)
{
    transform.SetPosition(position);
    UpdateViewMatrix();
    UpdateProjectionMatrix(aspectRatio);
}
Camera::Camera(
    float x, float y, float z,
    float aspectRatio,
    float movementSpeed,
    float mouseLookSpeed,
    float fieldOfView,
    float nearClip,
    float farClip,
    Mode mode) :
    aspectRatio(aspectRatio),
    movementSpeed(movementSpeed),
    mouseLookSpeed(mouseLookSpeed),
    fieldOfView(fieldOfView),
    nearClip(nearClip),
    farClip(farClip),
    mode(mode),
    orthographicWidth(5.0f)
{
    transform.SetPosition(x,y,z);
    UpdateViewMatrix();
    UpdateProjectionMatrix(aspectRatio);
}
Camera::~Camera(){}

void Camera::Update(float deltaTime)
{
    Input& input = Input::GetInstance(); // Make this a local var?

    // - Speed - //
    float distance = deltaTime * movementSpeed;
    if (input.KeyDown(VK_SHIFT)) { distance *= 5.0f; } // Speed up - sprint
    if (input.KeyDown(VK_CONTROL)) { distance *= 0.1f; } // Slow down - crouch

    // - Movement - //
    if (input.KeyDown('W')) { transform.MoveRelative(0, 0, distance); }
    if (input.KeyDown('S')) { transform.MoveRelative(0, 0, -distance); }
    if (input.KeyDown('A')) { transform.MoveRelative(-distance, 0, 0); }
    if (input.KeyDown('D')) { transform.MoveRelative(distance, 0, 0); }
    if (input.KeyDown('Q')) { transform.MoveAbsolute(0, -distance, 0); }
    if (input.KeyDown('E')) { transform.MoveAbsolute(0, distance, 0); }

    // - Mouse - //
    if (input.MouseLeftDown())
    {
        float xDiff = mouseLookSpeed * input.GetMouseXDelta(); // Yaw
        float yDiff = mouseLookSpeed * input.GetMouseYDelta(); // Pitch
        transform.Rotate(yDiff, xDiff, 0); // No rolling

        DirectX::XMFLOAT3 rotation = transform.GetRotation();
        float halfPiP1 = DirectX::XM_1DIV2PI+1.0f; // 90 deg
        float pi = 3.14f; // pi doesnt work?
        // prevent from going upside-down
        if (rotation.x > halfPiP1) rotation.x = halfPiP1;
        if (rotation.x < -halfPiP1) rotation.x = -halfPiP1;
        transform.SetRotation(rotation);
    }

    UpdateViewMatrix();
}
void Camera::UpdateViewMatrix()
{
    // Get
    DirectX::XMFLOAT3 position = transform.GetPosition();
    DirectX::XMFLOAT3 forward = transform.GetForward();
    //DirectX::XMFLOAT3 up = transform.GetUp();

    // Convert
    DirectX::XMVECTOR vPosition = DirectX::XMLoadFloat3(&position);
    DirectX::XMVECTOR vForward = DirectX::XMLoadFloat3(&forward);
    DirectX::XMVECTOR vUp = DirectX::XMVectorSet(0, 1, 0, 0); // World up axis

    // Calculate & Store
    DirectX::XMMATRIX view = DirectX::XMMatrixLookToLH(vPosition, vForward, vUp);
    DirectX::XMStoreFloat4x4(&viewMatrix, view); // Store
}
void Camera::UpdateProjectionMatrix(float aspectRatio)
{
    this->aspectRatio = aspectRatio;
    DirectX::XMMATRIX projection;

    switch (mode)
    {
    case Mode::PERSPECTIVE:
        projection = DirectX::XMMatrixPerspectiveFovLH(fieldOfView,aspectRatio,nearClip,farClip);
        break;
    case Mode::ORTHOGRAPHIC:
        float orthographicHeight = orthographicWidth / aspectRatio;
        projection = DirectX::XMMatrixOrthographicLH(orthographicWidth, orthographicHeight,nearClip,farClip);
        break;
    }

    DirectX::XMStoreFloat4x4(&projectionMatrix, projection);
}

// - Getters - //
Mode Camera::GetMode() { return mode; }
float Camera::GetOrthographicWidth() { return orthographicWidth; }
Transform* Camera::GetTransform() { return &transform; }
DirectX::XMFLOAT4X4 Camera::GetView() { return viewMatrix; }
DirectX::XMFLOAT4X4 Camera::GetProjection() { return projectionMatrix; }
float Camera::GetAspectRatio() { return aspectRatio; }
float Camera::GetFieldOfView() { return fieldOfView; }
float Camera::GetNearClip() { return nearClip; }
float Camera::GetFarClip() { return farClip; }
float Camera::GetMovementSpeed() { return movementSpeed; }
float Camera::GetMouseLookSpeed() { return mouseLookSpeed; }

// - Setters - //
void Camera::SetMode(Mode mode)
{
    this->mode = mode;
    UpdateProjectionMatrix(aspectRatio);
}
void Camera::SetOrthographicWidth(float width)
{
    orthographicWidth = width;
    UpdateProjectionMatrix(aspectRatio);
}
void Camera::SetAspectRatio(float aspectRatio)
{
    this->aspectRatio = aspectRatio;
    UpdateProjectionMatrix(aspectRatio);
}
void Camera::SetFieldOfView(float fieldOfView)
{
    this->fieldOfView = fieldOfView;
    UpdateProjectionMatrix(aspectRatio);
}
void Camera::SetNearClip(float nearDistance)
{
    nearClip = nearDistance;
    UpdateProjectionMatrix(aspectRatio);
}
void Camera::SetFarClip(float farDistance)
{
    farClip = farDistance;
    UpdateProjectionMatrix(aspectRatio);
}
void Camera::SetMovementSpeed(float movementSpeed)
{
    this->movementSpeed = movementSpeed;
}
void Camera::SetMouseLookSpeed(float mouseLookSpeed)
{
    this->mouseLookSpeed = mouseLookSpeed;
}
