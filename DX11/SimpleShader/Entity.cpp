#include "Entity.h"

Entity::Entity(std::shared_ptr<Mesh> mesh) : mesh(mesh) {}
Entity::Entity(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material) :
	mesh(mesh),
	material(material){}
Entity::~Entity() {}
void Entity::Draw(
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, 
	std::shared_ptr<Camera> camera)
{
	// Activate shaders
	material->GetVertexShader()->SetShader();
	std::shared_ptr<SimpleVertexShader> vs = material->GetVertexShader();
	vs->SetMatrix4x4("worldMatrix", transform.GetWorldMatrix());
	vs->SetMatrix4x4("viewMatrix", camera->GetView());
	vs->SetMatrix4x4("projectionMatrix", camera->GetProjection());
	vs->CopyAllBufferData();

	material->GetPixelShader()->SetShader();
	std::shared_ptr<SimplePixelShader> ps = material->GetPixelShader();
	ps->SetFloat3("colorTint", material->GetTint());
	ps->CopyAllBufferData();

	mesh->SetBuffersAndDraw(context);
}

// Getters
std::shared_ptr<Mesh> Entity::GetMesh(){ return mesh; }
std::shared_ptr<Material> Entity::GetMaterial(){ return material; }
Transform* Entity::GetTransform(){ return &transform; }

// Setters
void Entity::SetMesh(std::shared_ptr<Mesh> mesh){ this->mesh = mesh; }
void Entity::SetMaterial(std::shared_ptr<Material> material){ this->material = material; }
