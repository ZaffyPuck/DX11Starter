#include "HeaderStructs.hlsli"

// KEY
//Model/Object/Local space: The coordinates inside the model.
//World space: The coordinates in the world.
//Camera/View space	: The coordinates respect to the camera.
//Screen/Window/Device space: The coordinates for the screen.

//Model/Object matrix: from Model space to World space. You use this matrix to place objects in the world.
//View/Camera Transformation matrix: from World space to Camera space.
//Projection/Camera Projection Matrix: from Camera space to Clip (or screen?) space.

// Constant buffer
cbuffer ExternalData : register(b0)
{
	matrix worldMatrix;			// column-major - local->world space
    matrix worldInvTpsMatrix;	// Inverse Transpose World Matrix - world->local space
	matrix viewMatrix;			// Camera view - world->camera space - consists of camera transformation (translation+rotation+scale matrices) (SRT)
	matrix projectionMatrix;	// Camera proj - camera->screen space
	
    matrix shadowViewMatrix;
    matrix shadowProjectionMatrix;
}

VertexToPixel main( VertexShaderInput input )
{
	VertexToPixel output;		// Set up output struct
	matrix wvp = mul(projectionMatrix, mul(viewMatrix, worldMatrix)); // Multiply the three matrices together first
	output.screenPosition = mul(wvp, float4(input.localPosition, 1.0f));
	
    output.uv = input.uv;
    output.normal = normalize(mul((float3x3) worldInvTpsMatrix, input.normal)); // ->local space
    output.tangent = normalize(mul((float3x3) worldInvTpsMatrix, input.tangent));
    output.worldPosition = mul(worldMatrix, float4(input.localPosition, 1.0f)).xyz; // ->world space
	
    matrix shadowWVP = mul(shadowProjectionMatrix, mul(shadowViewMatrix, worldMatrix));
    output.shadowMapPos = mul(shadowWVP, float4(input.localPosition, 1.0f));
	
	return output;
}