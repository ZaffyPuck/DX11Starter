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
	material->Init(&transform, camera);
	mesh->SetBuffersAndDraw(context);
}

// Getters
std::shared_ptr<Mesh> Entity::GetMesh(){ return mesh; }
std::shared_ptr<Material> Entity::GetMaterial(){ return material; }
Transform* Entity::GetTransform(){ return &transform; }

// Setters
void Entity::SetMesh(std::shared_ptr<Mesh> mesh){ this->mesh = mesh; }
void Entity::SetMaterial(std::shared_ptr<Material> material){ this->material = material; }
