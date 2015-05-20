#include "gk2_meshLoader.h"
#include <vector>
#include "gk2_vertices.h"
#include <xnamath.h>
#include <fstream>

using namespace std;
using namespace gk2;

Mesh MeshLoader::GetSphere(int stacks, int slices, float radius /* = 0.5f */)
{
	int n = (stacks - 1) * slices + 2;
	vector<VertexPosNormal> vertices(n);
	vertices[0].Pos = XMFLOAT3(0.0f, radius, 0.0f);
	vertices[0].Normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
	float dp = XM_PI / stacks;
	float phi = dp;
	int k = 1;
	for (int i = 0; i < stacks - 1; ++i, phi += dp)
	{
		float cosp, sinp;
		XMScalarSinCos(&sinp, &cosp, phi);
		float thau = 0.0f;
		float dt = XM_2PI / slices;
		float stackR = radius * sinp;
		float stackY = radius * cosp;
		for (int j = 0; j < slices; ++j, thau += dt)
		{
			float cost, sint;
			XMScalarSinCos(&sint, &cost, thau);
			vertices[k].Pos = XMFLOAT3(stackR * cost, stackY, stackR * sint);
			vertices[k++].Normal = XMFLOAT3(cost * sinp, cosp, sint * sinp);
		}
	}
	vertices[k].Pos = XMFLOAT3(0.0f, -radius, 0.0f);
	vertices[k].Normal = XMFLOAT3(0.0f, -1.0f, 0.0f);
	int in = (stacks - 1) * slices * 6;
	vector<unsigned short> indices(in);
	k = 0;
	for (int j = 0; j < slices - 1; ++j)
	{
		indices[k++] = 0;
		indices[k++] = j + 2;
		indices[k++] = j + 1;
	}
	indices[k++] = 0;
	indices[k++] = 1;
	indices[k++] = slices;
	int i = 0;
	for (; i < stacks - 2; ++i)
	{
		int j = 0;
		for (; j < slices - 1; ++j)
		{
			indices[k++] = i*slices + j + 1;
			indices[k++] = i*slices + j + 2;
			indices[k++] = (i + 1)*slices + j + 2;
			indices[k++] = i*slices + j + 1;
			indices[k++] = (i + 1)*slices + j + 2;
			indices[k++] = (i + 1)*slices + j + 1;
		}
		indices[k++] = i*slices + j + 1;
		indices[k++] = i*slices + 1;
		indices[k++] = (i + 1)*slices + 1;
		indices[k++] = i*slices + j + 1;
		indices[k++] = (i + 1)*slices + 1;
		indices[k++] = (i + 1)*slices + j + 1;
	}
	for (int j = 0; j < slices - 1; ++j)
	{
		indices[k++] = i*slices + j + 1;
		indices[k++] = i*slices + j + 2;
		indices[k++] = n - 1;
	}
	indices[k++] = (i + 1)*slices;
	indices[k++] = i*slices + 1;
	indices[k++] = n - 1;
	return Mesh(m_device.CreateVertexBuffer(vertices), sizeof(VertexPosNormal),
		m_device.CreateIndexBuffer(indices), in);
}

Mesh MeshLoader::GetCylinder(int stacks, int slices, float radius /* = 0.5f */, float height /* = 1.0f */)
{
	int n = (stacks + 1) * slices * 2;
	vector<VertexPosNormal> vertices(n);
	float y = height / 2;
	float dy = height / stacks;
	float dp = XM_2PI / slices;
	int k = 0;
	for (int i = 0; i <= stacks; ++i, y -= dy)
	{
		float phi = 0.0f;
		for (int j = 0; j < slices; ++j, phi += dp)
		{
			float sinp, cosp;
			XMScalarSinCos(&sinp, &cosp, phi);
			vertices[k].Pos = XMFLOAT3(radius*cosp, y, radius*sinp);
			vertices[k++].Normal = XMFLOAT3(cosp, 0, sinp);
		}
	}
	y = height / 2;
	dy = height / stacks;
	dp = XM_2PI / slices;
	for (int i = 0; i <= stacks; ++i, y -= dy)
	{
		float phi = 0.0f;
		for (int j = 0; j < slices; ++j, phi += dp)
		{
			float sinp, cosp;
			XMScalarSinCos(&sinp, &cosp, phi);
			vertices[k].Pos = XMFLOAT3(radius*cosp, y, radius*sinp);
			vertices[k++].Normal = XMFLOAT3(0, 0, 0);
		}
	}
	int in = 6 * stacks * slices * 2;
	vector<unsigned short> indices(in);
	k = 0;
	for (int i = 0; i < stacks * 2; ++i)
	{
		int j = 0;
		for (; j < slices - 1; ++j)
		{
			indices[k++] = i*slices + j;
			indices[k++] = i*slices + j + 1;
			indices[k++] = (i + 1)*slices + j + 1;
			indices[k++] = i*slices + j;
			indices[k++] = (i + 1)*slices + j + 1;
			indices[k++] = (i + 1)*slices + j;
		}
		indices[k++] = i*slices + j;
		indices[k++] = i*slices;
		indices[k++] = (i + 1)*slices;
		indices[k++] = i*slices + j;
		indices[k++] = (i + 1)*slices;
		indices[k++] = (i + 1)*slices + j;
	}
	return Mesh(m_device.CreateVertexBuffer(vertices), sizeof(VertexPosNormal),
		m_device.CreateIndexBuffer(indices), in);
}

Mesh MeshLoader::GetBox(float side /* = 1.0f */)
{
	side /= 2;
	VertexPosNormal vertices[] =
	{
		//Front face
		{ XMFLOAT3(-side, -side, -side), XMFLOAT3(0.0f, 0.0f, -1.0f) },
		{ XMFLOAT3(-side, side, -side), XMFLOAT3(0.0f, 0.0f, -1.0f) },
		{ XMFLOAT3(side, side, -side), XMFLOAT3(0.0f, 0.0f, -1.0f) },
		{ XMFLOAT3(side, -side, -side), XMFLOAT3(0.0f, 0.0f, -1.0f) },

		//Left face
		{ XMFLOAT3(-side, -side, -side), XMFLOAT3(-1.0f, 0.0f, 0.0f) },
		{ XMFLOAT3(-side, -side, side), XMFLOAT3(-1.0f, 0.0f, 0.0f) },
		{ XMFLOAT3(-side, side, side), XMFLOAT3(-1.0f, 0.0f, 0.0f) },
		{ XMFLOAT3(-side, side, -side), XMFLOAT3(-1.0f, 0.0f, 0.0f) },

		//Bottom face
		{ XMFLOAT3(-side, -side, -side), XMFLOAT3(0.0f, -1.0f, 0.0f) },
		{ XMFLOAT3(side, -side, -side), XMFLOAT3(0.0f, -1.0f, 0.0f) },
		{ XMFLOAT3(side, -side, side), XMFLOAT3(0.0f, -1.0f, 0.0f) },
		{ XMFLOAT3(-side, -side, side), XMFLOAT3(0.0f, -1.0f, 0.0f) },

		//Back face
		{ XMFLOAT3(-side, -side, side), XMFLOAT3(0.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(side, -side, side), XMFLOAT3(0.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(side, side, side), XMFLOAT3(0.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(-side, side, side), XMFLOAT3(0.0f, 0.0f, 1.0f) },

		//Right face
		{ XMFLOAT3(side, -side, -side), XMFLOAT3(1.0f, 0.0f, 0.0f) },
		{ XMFLOAT3(side, side, -side), XMFLOAT3(1.0f, 0.0f, 0.0f) },
		{ XMFLOAT3(side, side, side), XMFLOAT3(1.0f, 0.0f, 0.0f) },
		{ XMFLOAT3(side, -side, side), XMFLOAT3(1.0f, 0.0f, 0.0f) },

		//Top face
		{ XMFLOAT3(-side, side, -side), XMFLOAT3(0.0f, 1.0f, 0.0f) },
		{ XMFLOAT3(-side, side, side), XMFLOAT3(0.0f, 1.0f, 0.0f) },
		{ XMFLOAT3(side, side, side), XMFLOAT3(0.0f, 1.0f, 0.0f) },
		{ XMFLOAT3(side, side, -side), XMFLOAT3(0.0f, 1.0f, 0.0f) },
	};
	unsigned short indices[] =
	{
		0, 1, 2, 0, 2, 3,		//Front face
		4, 5, 6, 4, 6, 7,		//Left face
		8, 9, 10, 8, 10, 11,	//Botton face
		12, 13, 14, 12, 14, 15,	//Back face
		16, 17, 18, 16, 18, 19,	//Right face
		20, 21, 22, 20, 22, 23	//Top face
	};
	return Mesh(m_device.CreateVertexBuffer(vertices, 24), sizeof(VertexPosNormal),
		m_device.CreateIndexBuffer(indices, 36), 36);
}

Mesh MeshLoader::GetQuad(float side /* = 1.0f */)
{
	side /= 2;
	VertexPosNormal vertices[] =
	{
		{ XMFLOAT3(-side, -side, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) },
		{ XMFLOAT3(-side, side, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) },
		{ XMFLOAT3(side, side, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) },
		{ XMFLOAT3(side, -side, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) }
	};
	unsigned short indices[] = { 0, 1, 2, 0, 2, 3 };
	return Mesh(m_device.CreateVertexBuffer(vertices, 4), sizeof(VertexPosNormal),
		m_device.CreateIndexBuffer(indices, 6), 6);
}

Mesh MeshLoader::GetCircle(int resolution, float radius)
{
	vector<VertexPosNormal> vertices(resolution);
	vector<unsigned short> indices(resolution * 2);

	for (int i = 0; i < resolution; ++i)
	{
		vertices[i].Pos.x = radius * cos(2 * XM_PI * i / resolution);
		vertices[i].Pos.z = radius * sin(2 * XM_PI * i / resolution);
		vertices[i].Normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
	}
	for (size_t i = 0; i < resolution; i++)
	{
		indices[i * 2] = i % resolution;
		indices[i * 2 + 1] = (i + 1) % resolution;
	}
	return Mesh(m_device.CreateVertexBuffer(vertices), sizeof(VertexPosNormal),
		m_device.CreateIndexBuffer(indices), resolution * 2);
}

Mesh MeshLoader::GetQuad(float width, float height)
{
	width /= 2;
	height /= 2;
	VertexPosNormal vertices[] =
	{
		{ XMFLOAT3(-width, -height, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) },
		{ XMFLOAT3(-width, height, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) },
		{ XMFLOAT3(width, height, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) },
		{ XMFLOAT3(width, -height, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) }
	};
	unsigned short indices[] = { 0, 1, 2, 0, 2, 3, 0, 2, 1, 0, 3, 2 };
	return Mesh(m_device.CreateVertexBuffer(vertices, 4), sizeof(VertexPosNormal),
		m_device.CreateIndexBuffer(indices, 12), 12);
}

Mesh MeshLoader::GetRim(int slices, float radius /* = 0.5f */)
{
	int n = slices;
	vector<VertexPosNormal> vertices(2 * n);
	float phi = 0.0f;
	float dp = XM_2PI / slices;
	int k = 0;
	for (int i = 0; i < slices; ++i, phi += dp)
	{
		float cosp, sinp;
		XMScalarSinCos(&sinp, &cosp, phi);
		vertices[k].Pos = XMFLOAT3(radius * cosp, 0.0f, radius * sinp);
		vertices[k++].Normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
	}
	for (int i = 0; i < slices; ++i, phi += dp)
	{
		float cosp, sinp;
		XMScalarSinCos(&sinp, &cosp, phi);
		vertices[k].Pos = XMFLOAT3((radius - 0.03) * cosp, 0.0f, (radius - 0.03) * sinp);
		vertices[k++].Normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
	}
	int in = slices * 6;
	vector<unsigned short> indices(in);
	k = 0;
	for (int i = 0; i < n - 1; i++)
	{
		indices[k++] = (n + i);
		indices[k++] = (i + 1);
		indices[k++] = i;
		indices[k++] = (n + i);
		indices[k++] = (n + i + 1);
		indices[k++] = (i + 1);
	}
	indices[k++] = 2 * n - 1;
	indices[k++] = 0;
	indices[k++] = n - 1;
	indices[k++] = 2 * n - 1;
	indices[k++] = n;
	indices[k++] = 0;
	return Mesh(m_device.CreateVertexBuffer(vertices), sizeof(VertexPosNormal),
		m_device.CreateIndexBuffer(indices), in);
}

Mesh MeshLoader::GetDisc(int slices, float radius /* = 0.5f */)
{
	int n = slices + 1;
	vector<VertexPosNormal> vertices(n);
	vertices[0].Pos = XMFLOAT3(0.0f, 0.0f, 0.0f);
	vertices[0].Normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
	float phi = 0.0f;
	float dp = XM_2PI / slices;
	int k = 1;
	for (int i = 1; i <= slices; ++i, phi += dp)
	{
		float cosp, sinp;
		XMScalarSinCos(&sinp, &cosp, phi);
		vertices[k].Pos = XMFLOAT3(radius * cosp, 0.0f, radius * sinp);
		vertices[k++].Normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
	}
	int in = slices * 3;
	vector<unsigned short> indices(in);
	k = 0;
	for (int i = 0; i < slices - 1; ++i)
	{
		indices[k++] = 0;
		indices[k++] = i + 2;
		indices[k++] = i + 1;
	}
	indices[k++] = 0;
	indices[k++] = 1;
	indices[k++] = slices;
	return Mesh(m_device.CreateVertexBuffer(vertices), sizeof(VertexPosNormal),
		m_device.CreateIndexBuffer(indices), in);
}

Mesh MeshLoader::LoadMesh(const wstring& fileName)
{
	ifstream input;
	input.exceptions(ios::badbit | ios::failbit | ios::eofbit); //Most of the time you really shouldn't throw
	//exceptions in case of eof, but here if end of file was
	//reached before the whole mesh was loaded, we would
	//have had to throw an exception anyway.
	int n, in;
	input.open(fileName);
	input >> n >> in;
	vector<VertexPosNormal> vertices(n);
	XMFLOAT2 texDummy;
	for (int i = 0; i < n; ++i)
	{
		input >> vertices[i].Pos.x >> vertices[i].Pos.y >> vertices[i].Pos.z;
		input >> vertices[i].Normal.x >> vertices[i].Normal.y >> vertices[i].Normal.z;
		input >> texDummy.x >> texDummy.y;
	}
	vector<unsigned short> indices(in);
	for (int i = 0; i < in; ++i)
		input >> indices[i];
	input.close();
	return Mesh(m_device.CreateVertexBuffer(vertices), sizeof(VertexPosNormal),
		m_device.CreateIndexBuffer(indices), in);
}


Mesh MeshLoader::LoadMeshForPuma(const wstring& fileName, Mesh& shadowVolume, XMFLOAT4 lightPosition)
{
	ifstream input;
	input.exceptions(ios::badbit | ios::failbit | ios::eofbit); //Most of the time you really shouldn't throw
	//exceptions in case of eof, but here if end of file was
	//reached before the whole mesh was loaded, we would
	//have had to throw an exception anyway.
	int vert_count, differences_vert_count;
	input.open(fileName);

	input >> vert_count;
	vector<VertexPosNormal>  vertices(vert_count);
	for (int i = 0; i < vert_count; i++){
		input >> vertices[i].Pos.x >> vertices[i].Pos.y >> vertices[i].Pos.z;
	}

	input >> differences_vert_count;
	vector<VertexPosNormal> diff_vertices(differences_vert_count);
	for (int i = 0; i < differences_vert_count; i++){
		int ix;
		input >> ix;
		diff_vertices[i] = vertices[ix];

		XMFLOAT3 a;
		input >> a.x >> a.y >> a.z;
		XMVECTOR A = XMLoadFloat3(&a);
		A = XMVector3Normalize(A);
		XMStoreFloat3(&a, A);
		diff_vertices[i].Normal.x = a.x;
		diff_vertices[i].Normal.y = a.y;
		diff_vertices[i].Normal.z = a.z;
	}

	int trianglesCount;
	input >> trianglesCount;
	vector<unsigned short> indices(trianglesCount * 3);
	for (int i = 0; i < trianglesCount; ++i){
		input >> indices[3 * i] >> indices[3 * i + 1] >> indices[3 * i + 2];
	}

	int edgesCount;
	input >> edgesCount;
	auto edges = vector<int[4]>(edgesCount);
	for (int i = 0; i < edgesCount; ++i)
		input >> edges[i][0] >> edges[i][1] >> edges[i][2] >> edges[i][3];

	vector<vector<XMFLOAT3>> borders;
	for (int i = 0; i < edgesCount; ++i)
	{
		XMFLOAT3 vBeg = vertices[edges[i][0]].Pos;
		XMFLOAT3 vEnd = vertices[edges[i][1]].Pos;

		XMFLOAT3 vL0 = diff_vertices[indices[edges[i][2] * 3]].Pos;
		XMFLOAT3 vL1 = diff_vertices[indices[edges[i][2] * 3 + 1]].Pos;
		XMFLOAT3 vL2 = diff_vertices[indices[edges[i][2] * 3 + 2]].Pos;
		XMFLOAT3 cL0 = XMFLOAT3(vL1.x - vL0.x, vL1.y - vL0.y, vL1.z - vL0.z);
		XMFLOAT3 cL1 = XMFLOAT3(vL2.x - vL0.x, vL2.y - vL0.y, vL2.z - vL0.z);

		XMFLOAT3 vR0 = diff_vertices[indices[edges[i][3] * 3]].Pos;
		XMFLOAT3 vR1 = diff_vertices[indices[edges[i][3] * 3 + 1]].Pos;
		XMFLOAT3 vR2 = diff_vertices[indices[edges[i][3] * 3 + 2]].Pos;
		XMFLOAT3 cR0 = XMFLOAT3(vR1.x - vR0.x, vR1.y - vR0.y, vR1.z - vR0.z);
		XMFLOAT3 cR1 = XMFLOAT3(vR2.x - vR0.x, vR2.y - vR0.y, vR2.z - vR0.z);

		XMVECTOR tLnormal = XMVector3Cross(XMLoadFloat3(&cL0), XMLoadFloat3(&cL1));
		XMVECTOR tRnormal = XMVector3Cross(XMLoadFloat3(&cR0), XMLoadFloat3(&cR1));

		XMVECTOR light = XMLoadFloat3(&XMFLOAT3(lightPosition.x - vBeg.x,
			lightPosition.y - vBeg.y, lightPosition.z - vBeg.z));

		float tLDot = XMVector3Dot(tLnormal, light).m128_f32[0];
		float tRDot = XMVector3Dot(tRnormal, light).m128_f32[0];
		if ((tLDot > 0 && tRDot <= 0) || (tLDot <= 0 && tRDot > 0))
			borders.push_back(vector < XMFLOAT3 > {vBeg, vEnd});
	}

	vector<VertexPosNormal> volumeVertices(borders.size() * 4);
	vector<unsigned short> volumeIndices(borders.size() * 12);
	int vInc = 0;
	int iInc = 0;

	for (int i = 0; i < borders.size(); ++i)
	{
		XMFLOAT3 vBeg = borders[i][0];
		XMFLOAT3 vEnd = borders[i][1];
		volumeVertices[vInc++].Pos = vBeg;
		volumeVertices[vInc++].Pos = vEnd;

		vBeg = XMFLOAT3(vBeg.x - lightPosition.x, vBeg.y - lightPosition.y, vBeg.z - lightPosition.z);
		vEnd = XMFLOAT3(vEnd.x - lightPosition.x, vEnd.y - lightPosition.y, vEnd.z - lightPosition.z);
		volumeVertices[vInc++].Pos = vBeg;
		volumeVertices[vInc++].Pos = vEnd;

		volumeIndices[iInc++] = 3 * i;
		volumeIndices[iInc++] = 3 * i + 1;
		volumeIndices[iInc++] = 3 * i + 2;

		volumeIndices[iInc++] = 3 * i + 1;
		volumeIndices[iInc++] = 3 * i + 3;
		volumeIndices[iInc++] = 3 * i + 2;

		volumeIndices[iInc++] = 3 * i + 2;
		volumeIndices[iInc++] = 3 * i + 1;
		volumeIndices[iInc++] = 3 * i;

		volumeIndices[iInc++] = 3 * i + 2;
		volumeIndices[iInc++] = 3 * i + 3;
		volumeIndices[iInc++] = 3 * i + 1;
	}
	shadowVolume = Mesh(m_device.CreateVertexBuffer(volumeVertices), sizeof(VertexPosNormal), m_device.CreateIndexBuffer(volumeIndices),
		volumeIndices.size());

	input.close();

	return Mesh(m_device.CreateVertexBuffer(diff_vertices), sizeof(VertexPosNormal), m_device.CreateIndexBuffer(indices), trianglesCount * 3);
}
