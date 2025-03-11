#pragma once
#include <wrl/client.h>
#include <DirectXMath.h>
#include <memory> // shared pointer

#include "Transform.h"
#include "Mesh.h"
#include "BufferStructs.h"
#include "Camera.h"

class Entity
{
private:
	Transform transform;
	std::shared_ptr<Mesh> mesh;
public:
	Entity(std::shared_ptr<Mesh> mesh);
	~Entity();
	void Draw(
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, 
		Microsoft::WRL::ComPtr<ID3D11Buffer> vsConstantBuffer, 
		std::shared_ptr<Camera> camera);

	std::shared_ptr<Mesh> GetMesh();
	Transform* GetTransform();

	void SetMesh(std::shared_ptr<Mesh> mesh);
};

