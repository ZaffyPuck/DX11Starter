#include "GameObject.h"

GameObject::GameObject(std::shared_ptr<Mesh> mesh):
    mesh(mesh)
{
}

std::shared_ptr<Mesh> GameObject::GetMesh()
{
    return mesh;
}
Transform* GameObject::GetTransform()
{
    return &transform;
}

void GameObject::SetMesh(std::shared_ptr<Mesh> mesh)
{
    this->mesh = mesh;
}
