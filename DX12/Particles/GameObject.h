#pragma once
#include <wrl/client.h>
#include <DirectXMath.h>
#include <memory>
#include "Camera.h"
#include "Transform.h"
#include "Mesh.h"
#include "Material.h"

class GameObject
{
private:
	Transform transform;
	std::shared_ptr<Mesh> mesh;
	std::shared_ptr<Material> material;
public:
	GameObject(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material);

	// Getters
	Transform* GetTransform();
	std::shared_ptr<Mesh> GetMesh();
	std::shared_ptr<Material> GetMaterial();

	// Setters
	void SetMesh(std::shared_ptr<Mesh> mesh);
	void SetMaterial(std::shared_ptr<Material> material);
};

