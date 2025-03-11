#include "Material.h"

Material::Material(DirectX::XMFLOAT3 tint, 
    std::shared_ptr<SimpleVertexShader> vertexShader, 
    std::shared_ptr<SimplePixelShader> pixelShader,
    float roughness) :
tint(tint),
vertexShader(vertexShader),
pixelShader(pixelShader),
roughness(roughness){}
Material::~Material(){}

// Getters
DirectX::XMFLOAT3 Material::GetTint() { return tint; }
std::shared_ptr<SimpleVertexShader> Material::GetVertexShader() { return vertexShader; }
std::shared_ptr<SimplePixelShader> Material::GetPixelShader() { return pixelShader; }
float Material::GetRoughness() { return roughness; }

// Setters
void Material::SetTint(DirectX::XMFLOAT3 tint)
{ this->tint = tint; }
void Material::SetVertexShader(std::shared_ptr<SimpleVertexShader> vertexShader) 
{ this->vertexShader = vertexShader; }
void Material::SetPixelShader(std::shared_ptr<SimplePixelShader> pixelShader)
{ this->pixelShader = pixelShader; }
void Material::SetRoughness(float roughness)
{this->roughness = roughness;}