#include "GameObject.h"

GameObject::GameObject(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material):
    mesh(mesh),
    material(material)
{
}

Transform* GameObject::GetTransform()
{
    return &transform;
}
std::shared_ptr<Mesh> GameObject::GetMesh()
{
    return mesh;
}
std::shared_ptr<Material> GameObject::GetMaterial()
{
    return material;
}

void GameObject::SetMesh(std::shared_ptr<Mesh> mesh)
{
    this->mesh = mesh;
}
void GameObject::SetMaterial(std::shared_ptr<Material> material)
{
    this->material = material;
}
