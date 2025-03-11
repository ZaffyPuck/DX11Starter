#include "Material.h"

Material::Material(DirectX::XMFLOAT3 tint,
	std::shared_ptr<SimpleVertexShader> vertexShader,
	std::shared_ptr<SimplePixelShader> pixelShader,
	float roughness) :
	tint(tint),
	vertexShader(vertexShader),
	pixelShader(pixelShader),
	roughness(roughness),
	uvScale(DirectX::XMFLOAT2(1, 1)),
	uvOffset(DirectX::XMFLOAT2(0, 0)) {};
Material::Material(DirectX::XMFLOAT3 tint,
	std::shared_ptr<SimpleVertexShader> vertexShader,
	std::shared_ptr<SimplePixelShader> pixelShader,
	float roughness,
	DirectX::XMFLOAT2 uvScale,
	DirectX::XMFLOAT2 uvOffset) :
	tint(tint),
	vertexShader(vertexShader),
	pixelShader(pixelShader),
	roughness(roughness),
	uvScale(uvScale),
	uvOffset(uvOffset) {};
Material::~Material() {};
void Material::Init(Transform* transform, std::shared_ptr<Camera> camera)
{
	// Activate shaders
	vertexShader->SetShader();
	std::shared_ptr<SimpleVertexShader> vs = vertexShader;
	vs->SetMatrix4x4("worldMatrix", transform->GetWorldMatrix());
	vs->SetMatrix4x4("viewMatrix", camera->GetView());
	vs->SetMatrix4x4("projectionMatrix", camera->GetProjection());
	vs->SetMatrix4x4("worldInvTpsMatrix", transform->GetWorldInverseTransposeMatrix());
	vs->CopyAllBufferData();

	pixelShader->SetShader();
	std::shared_ptr<SimplePixelShader> ps = pixelShader;
	ps->SetFloat3("colorTint", tint);
	ps->SetFloat("roughness", roughness);
	ps->SetFloat3("cameraPosition", camera->GetTransform()->GetPosition());
	ps->SetFloat2("uvScale", uvScale);
	ps->SetFloat2("uvOffset", uvOffset);
	ps->CopyAllBufferData();

	for (auto& t : textureSRVs)
	{ ps->SetShaderResourceView(t.first.c_str(), t.second); }
	for (auto& s : samplers) 
	{ ps->SetSamplerState(s.first.c_str(), s.second); }
}

// Getters
DirectX::XMFLOAT3 Material::GetTint() { return tint; }
std::shared_ptr<SimpleVertexShader> Material::GetVertexShader() { return vertexShader; }
std::shared_ptr<SimplePixelShader> Material::GetPixelShader() { return pixelShader; }
float Material::GetRoughness() { return roughness; }
DirectX::XMFLOAT2 Material::GetUVScale(){return uvScale;}
DirectX::XMFLOAT2 Material::GetUVOffset() {return uvOffset;}
Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Material::GetTextureSRV(std::string name){ return textureSRVs[name]; }
Microsoft::WRL::ComPtr<ID3D11SamplerState> Material::GetSampler(std::string name) { return samplers[name]; }

// Setters
void Material::SetTint(DirectX::XMFLOAT3 tint)
{ this->tint = tint; }
void Material::SetVertexShader(std::shared_ptr<SimpleVertexShader> vertexShader) 
{ this->vertexShader = vertexShader; }
void Material::SetPixelShader(std::shared_ptr<SimplePixelShader> pixelShader)
{ this->pixelShader = pixelShader; }
void Material::SetRoughness(float roughness)
{this->roughness = roughness;}
void Material::SetUVScale(DirectX::XMFLOAT2 scale)
{this->uvScale = scale;}
void Material::SetUVOffset(DirectX::XMFLOAT2 offset)
{this->uvOffset = offset;}

// Textures
void Material::AddTextureSRV(std::string name, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv)
{textureSRVs.insert({ name, srv });}
void Material::AddSampler(std::string name, Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler)
{samplers.insert({ name, sampler });}
void Material::RemoveTextureSRV(std::string name)
{textureSRVs.erase(name);}
void Material::RemoveSampler(std::string name)
{samplers.erase(name);}

