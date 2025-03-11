#include "Entity.h"

Entity::Entity(std::shared_ptr<Mesh> mesh) : mesh(mesh) {}
Entity::~Entity() {}
void Entity::Draw(
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, 
	Microsoft::WRL::ComPtr<ID3D11Buffer> vsConstantBuffer,
	std::shared_ptr<Camera> camera)
{
	VertexShaderExternalData vsData = {};
	vsData.colorTint = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	vsData.worldMatrix = transform.GetWorldMatrix();
	vsData.viewMatrix = camera->GetView();
	vsData.projectionMatrix = camera->GetProjection();

	D3D11_MAPPED_SUBRESOURCE mappedBuffer = {};
	context->Map(vsConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedBuffer); // lock from GPU
	memcpy(mappedBuffer.pData, &vsData, sizeof(vsData));
	context->Unmap(vsConstantBuffer.Get(), 0); // unlock from GPU
	context->VSSetConstantBuffers(0,1,vsConstantBuffer.GetAddressOf()); // Array of buffers (or the address of one) // prof does not have this?
	mesh->SetBuffersAndDraw(context); // Doesn't set const buffer (^^)
}

// Getters
std::shared_ptr<Mesh> Entity::GetMesh()
{
    return mesh;
}
Transform* Entity::GetTransform()
{
    //std::shared_ptr<Transform> GetTransform() // Shared pointer version
    //Transform* GetTransform()                 // Raw pointer version
    //Transform& GetTransform()                 // Reference version
    return &transform;
}

// Setters
void Entity::SetMesh(std::shared_ptr<Mesh> mesh)
{
    this->mesh = mesh;
}
