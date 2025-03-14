#pragma once
#include <wrl/client.h>
#include <DirectXMath.h>
#include <memory> // shared pointer

#include "Transform.h"
#include "Mesh.h"
#include "Camera.h"
#include "Material.h"

class Entity
{
private:
	Transform transform;
	std::shared_ptr<Mesh> mesh;
	std::shared_ptr<Material> material;
public:
	Entity(std::shared_ptr<Mesh> mesh);
	Entity(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material);
	~Entity();
	void Draw(
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, 
		std::shared_ptr<Camera> camera);

	std::shared_ptr<Mesh> GetMesh();
	std::shared_ptr<Material> GetMaterial();
	Transform* GetTransform();

	void SetMesh(std::shared_ptr<Mesh> mesh);
	void SetMaterial(std::shared_ptr<Material> material);
};

