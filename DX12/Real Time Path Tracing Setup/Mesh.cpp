#include <DirectXMath.h>
#include <vector>
#include <fstream>

#include "Mesh.h"
#include "DX12Helper.h"
#include "RaytracingHelper.h"

using namespace DirectX;

// --------------------------------------------------------
// Creates a new mesh with the given geometry
//
// vertArray  - An array of vertices
// numVerts   - The number of verts in the array
// indexArray - An array of indices into the vertex array
// numIndices - The number of indices in the index array
// device     - The D3D device to use for buffer creation
// --------------------------------------------------------
Mesh::Mesh(Vertex* vertices, size_t vertexCount, UINT* indices, size_t indexCount)
{
	CreateBuffers(vertices, vertexCount, indices, indexCount);
}

// --------------------------------------------------------
// Creates a new mesh by loading vertices from the given .obj file
//
// objFile  - Path to the .obj 3D model file to load
// device   - The D3D device to use for buffer creation
// --------------------------------------------------------
Mesh::Mesh(const std::wstring& objFile) :
ibView{},
vbView{},
indexCount(0),
vertexCount(0)
{
	// File input object
	std::ifstream obj(objFile);

	// Check for successful open
	if (!obj.is_open())
		return;

	// Variables used while reading the file
	std::vector<XMFLOAT3> positions;     // Positions from the file
	std::vector<XMFLOAT3> normals;       // Normals from the file
	std::vector<XMFLOAT2> uvs;           // UVs from the file
	std::vector<Vertex> vertices;        // Verts we're assembling
	std::vector<UINT> indices;           // Indices of these verts
	UINT vertCounter = 0;        // Count of vertices/indices
	char chars[100];                     // String for line reading

	// Still have data left?
	while (obj.good())
	{
		// Get the line (100 characters should be more than enough)
		obj.getline(chars, 100);

		// Check the type of line
		if (chars[0] == 'v' && chars[1] == 'n')
		{
			// Read the 3 numbers directly into an XMFLOAT3
			XMFLOAT3 norm = { 0, 0, 0 };
			sscanf_s(
				chars,
				"vn %f %f %f",
				&norm.x, &norm.y, &norm.z);

			// Add to the list of normals
			normals.push_back(norm);
		}
		else if (chars[0] == 'v' && chars[1] == 't')
		{
			// Read the 2 numbers directly into an XMFLOAT2
			XMFLOAT2 uv = { 0, 0 };
			sscanf_s(
				chars,
				"vt %f %f",
				&uv.x, &uv.y);

			// Add to the list of uv's
			uvs.push_back(uv);
		}
		else if (chars[0] == 'v')
		{
			// Read the 3 numbers directly into an XMFLOAT3
			XMFLOAT3 pos = { 0, 0, 0 };
			sscanf_s(
				chars,
				"v %f %f %f",
				&pos.x, &pos.y, &pos.z);

			// Add to the positions
			positions.push_back(pos);
		}
		else if (chars[0] == 'f')
		{
			// Read the face indices into an array
			// NOTE: This assumes the given obj file contains
			//  vertex positions, uv coordinates AND normals.
			//  If the model is missing any of these, this
			//  code will not handle the file correctly!
			unsigned int i[12] = {};
			int facesRead = sscanf_s(
				chars,
				"f %d/%d/%d %d/%d/%d %d/%d/%d %d/%d/%d",
				&i[0], &i[1], &i[2],
				&i[3], &i[4], &i[5],
				&i[6], &i[7], &i[8],
				&i[9], &i[10], &i[11]);

			// - Create the verts by looking up
			//    corresponding data from vectors
			// - OBJ File indices are 1-based, so
			//    they need to be adusted
			Vertex v1 = {};
			v1.Position = positions[max(i[0] - 1, 0)];
			v1.UV = uvs[max(i[1] - 1, 0)];
			v1.Normal = normals[max(i[2] - 1, 0)];

			Vertex v2 = {};
			v2.Position = positions[max(i[3] - 1, 0)];
			v2.UV = uvs[max(i[4] - 1, 0)];
			v2.Normal = normals[max(i[5] - 1, 0)];

			Vertex v3 = {};
			v3.Position = positions[max(i[6] - 1, 0)];
			v3.UV = uvs[max(i[7] - 1, 0)];
			v3.Normal = normals[max(i[8] - 1, 0)];

			// The model is most likely in a right-handed space,
			// especially if it came from Maya.  We want to convert
			// to a left-handed space for DirectX.  This means we
			// need to:
			//  - Invert the Z position
			//  - Invert the normal's Z
			//  - Flip the winding order
			// We also need to flip the UV coordinate since DirectX
			// defines (0,0) as the top left of the texture, and many
			// 3D modeling packages use the bottom left as (0,0)

			// Flip the UV's since they're probably "upside down"
			v1.UV.y = 1.0f - v1.UV.y;
			v2.UV.y = 1.0f - v2.UV.y;
			v3.UV.y = 1.0f - v3.UV.y;

			// Flip Z (LH vs. RH)
			v1.Position.z *= -1.0f;
			v2.Position.z *= -1.0f;
			v3.Position.z *= -1.0f;

			// Flip normal Z
			v1.Normal.z *= -1.0f;
			v2.Normal.z *= -1.0f;
			v3.Normal.z *= -1.0f;

			// Add the verts to the vector (flipping the winding order)
			vertices.push_back(v1);
			vertices.push_back(v3);
			vertices.push_back(v2);

			// Add three more indices
			indices.push_back(vertCounter); vertCounter += 1;
			indices.push_back(vertCounter); vertCounter += 1;
			indices.push_back(vertCounter); vertCounter += 1;

			// Was there a 4th face?
			if (facesRead == 12)
			{
				// Make the last vertex
				Vertex v4 = {};
				v4.Position = positions[max(i[9] - 1, 0)];
				v4.UV = uvs[max(i[10] - 1, 0)];
				v4.Normal = normals[max(i[11] - 1, 0)];

				// Flip the UV, Z pos and normal
				v4.UV.y = 1.0f - v4.UV.y;
				v4.Position.z *= -1.0f;
				v4.Normal.z *= -1.0f;

				// Add a whole triangle (flipping the winding order)
				vertices.push_back(v1);
				vertices.push_back(v4);
				vertices.push_back(v3);

				// Add three more indices
				indices.push_back(vertCounter); vertCounter += 1;
				indices.push_back(vertCounter); vertCounter += 1;
				indices.push_back(vertCounter); vertCounter += 1;
			}
		}
	}

	// Close the file and create the actual buffers
	obj.close();

	// Will vertices.data() and indices.data() work?
	CreateBuffers(&vertices[0], vertCounter, &indices[0], vertCounter);
}


void Mesh::CreateBuffers(Vertex* vertices, size_t vertexCount, UINT* indices, size_t indexCount)
{
	this->indexCount = indexCount;
	this->vertexCount = vertexCount;

	// Calculate the tangents before copying to buffer
	CalculateTangents(vertices, vertexCount, indices, indexCount);

	// Create the two buffers
	DX12Helper& dx12Utility = DX12Helper::GetInstance();
	vertexBuffer = dx12Utility.CreateStaticBuffer(sizeof(Vertex), vertexCount, vertices);
	indexBuffer = dx12Utility.CreateStaticBuffer(sizeof(unsigned int), indexCount, indices);

	// Set up the views
	vbView.StrideInBytes = sizeof(Vertex);
	vbView.SizeInBytes = sizeof(Vertex) * vertexCount;
	vbView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();

	ibView.Format = DXGI_FORMAT_R32_UINT;
	ibView.SizeInBytes = sizeof(unsigned int) * indexCount;
	ibView.BufferLocation = indexBuffer->GetGPUVirtualAddress();

	// Create the raytracing acceleration structure for this mesh
	raytracingData = RaytracingHelper::GetInstance().CreateBottomLevelAccelerationStructureForMesh(this);
}

// --------------------------------------------------------
// Calculates the tangents of the vertices in a mesh
// - Code originally adapted from: http://www.terathon.com/code/tangent.html
//   - Updated version now found here: http://foundationsofgameenginedev.com/FGED2-sample.pdf
//   - See listing 7.4 in section 7.5 (page 9 of the PDF)
//
// - Note: For this code to work, your Vertex format must
//         contain an XMFLOAT3 called Tangent
//
// - Be sure to call this BEFORE creating your D3D vertex/index buffers
// --------------------------------------------------------
void Mesh::CalculateTangents(Vertex* vertices, size_t vertexCount, UINT* indices, size_t indexCount)
{
	// Reset tangents
	for (int i = 0; i < vertexCount; i++)
	{
		vertices[i].Tangent = XMFLOAT3(0, 0, 0);
	}

	// Calculate tangents one whole triangle at a time
	for (int i = 0; i < vertexCount;)
	{
		// Grab indices and vertices of first triangle
		unsigned int i1 = indices[i++];
		unsigned int i2 = indices[i++];
		unsigned int i3 = indices[i++];
		Vertex* v1 = &vertices[i1];
		Vertex* v2 = &vertices[i2];
		Vertex* v3 = &vertices[i3];

		// Calculate vectors relative to triangle positions
		float x1 = v2->Position.x - v1->Position.x;
		float y1 = v2->Position.y - v1->Position.y;
		float z1 = v2->Position.z - v1->Position.z;

		float x2 = v3->Position.x - v1->Position.x;
		float y2 = v3->Position.y - v1->Position.y;
		float z2 = v3->Position.z - v1->Position.z;

		// Do the same for vectors relative to triangle uv's
		float s1 = v2->UV.x - v1->UV.x;
		float t1 = v2->UV.y - v1->UV.y;

		float s2 = v3->UV.x - v1->UV.x;
		float t2 = v3->UV.y - v1->UV.y;

		// Create vectors for tangent calculation
		float r = 1.0f / (s1 * t2 - s2 * t1);

		float tx = (t2 * x1 - t1 * x2) * r;
		float ty = (t2 * y1 - t1 * y2) * r;
		float tz = (t2 * z1 - t1 * z2) * r;

		// Adjust tangents of each vert of the triangle
		v1->Tangent.x += tx;
		v1->Tangent.y += ty;
		v1->Tangent.z += tz;

		v2->Tangent.x += tx;
		v2->Tangent.y += ty;
		v2->Tangent.z += tz;

		v3->Tangent.x += tx;
		v3->Tangent.y += ty;
		v3->Tangent.z += tz;
	}

	// Ensure all of the tangents are orthogonal to the normals
	for (int i = 0; i < vertexCount; i++)
	{
		// Grab the two vectors
		XMVECTOR normal = XMLoadFloat3(&vertices[i].Normal);
		XMVECTOR tangent = XMLoadFloat3(&vertices[i].Tangent);

		// Use Gram-Schmidt orthonormalize to ensure
		// the normal and tangent are exactly 90 degrees apart
		tangent = XMVector3Normalize(
			tangent - normal * XMVector3Dot(normal, tangent));

		// Store the tangent
		XMStoreFloat3(&vertices[i].Tangent, tangent);
	}
}