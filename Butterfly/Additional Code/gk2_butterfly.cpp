#include "gk2_butterfly.h"
#include "gk2_utils.h"
#include "gk2_vertices.h"
#include "gk2_window.h"

using namespace std;
using namespace gk2;

#define RESOURCES_PATH L"resources/"
const wstring Butterfly::ShaderFile = RESOURCES_PATH L"shaders/Butterfly.hlsl";

const float Butterfly::DODECAHEDRON_R = sqrtf(0.375f + 0.125f * sqrtf(5.0f));
const float Butterfly::DODECAHEDRON_H = 1.0f + 2.0f * Butterfly::DODECAHEDRON_R;
const float Butterfly::DODECAHEDRON_A = XMScalarACos(-0.2f * sqrtf(5.0f));

const float Butterfly::MOEBIUS_R = 1.0f;
const float Butterfly::MOEBIUS_W = 0.1f;
const int Butterfly::MOEBIUS_N = 128;

const float Butterfly::LAP_TIME = 10.0f;
const float Butterfly::FLAP_TIME = 2.0f;
const float Butterfly::WING_W = 0.15f;
const float Butterfly::WING_H = 0.1f;
const float Butterfly::WING_MAX_A = 8.0f * XM_PIDIV2 / 9.0f; //80 degrees
const XMFLOAT4 Butterfly::BLUE_LIGHT_POS = XMFLOAT4(-1.0f, -1.0f, -1.0f, 1.0f);
const XMFLOAT4 Butterfly::GREEN_LIGHT_POS = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

const unsigned int Butterfly::VB_STRIDE = sizeof(VertexPosNormal);
const unsigned int Butterfly::VB_OFFSET = 0;
const unsigned int Butterfly::BS_MASK = 0xffffffff;

const XMFLOAT4 Butterfly::COLORS[] =
{
	XMFLOAT4(253.0f / 255.0f, 198.0f / 255.0f, 137.0f / 255.0f, 100.0f / 255.0f),
	XMFLOAT4(255.0f / 255.0f, 247.0f / 255.0f, 153.0f / 255.0f, 100.0f / 255.0f),
	XMFLOAT4(196.0f / 255.0f, 223.0f / 255.0f, 155.0f / 255.0f, 100.0f / 255.0f),
	XMFLOAT4(162.0f / 255.0f, 211.0f / 255.0f, 156.0f / 255.0f, 100.0f / 255.0f),
	XMFLOAT4(130.0f / 255.0f, 202.0f / 255.0f, 156.0f / 255.0f, 100.0f / 255.0f),
	XMFLOAT4(122.0f / 255.0f, 204.0f / 255.0f, 200.0f / 255.0f, 100.0f / 255.0f),
	XMFLOAT4(109.0f / 255.0f, 207.0f / 255.0f, 246.0f / 255.0f, 100.0f / 255.0f),
	XMFLOAT4(125.0f / 255.0f, 167.0f / 255.0f, 216.0f / 255.0f, 100.0f / 255.0f),
	XMFLOAT4(131.0f / 255.0f, 147.0f / 255.0f, 202.0f / 255.0f, 100.0f / 255.0f),
	XMFLOAT4(135.0f / 255.0f, 129.0f / 255.0f, 189.0f / 255.0f, 100.0f / 255.0f),
	XMFLOAT4(161.0f / 255.0f, 134.0f / 255.0f, 190.0f / 255.0f, 100.0f / 255.0f),
	XMFLOAT4(244.0f / 255.0f, 154.0f / 255.0f, 193.0f / 255.0f, 100.0f / 255.0f)
};

void* Butterfly::operator new(size_t size)
{
	return Utils::New16Aligned(size);
}

void Butterfly::operator delete(void* ptr)
{
	Utils::Delete16Aligned(ptr);
}

Butterfly::Butterfly(HINSTANCE hInstance)
	: ApplicationBase(hInstance), m_camera(0.01f, 100.0f)
{

}

Butterfly::~Butterfly()
{

}

void Butterfly::InitializeShaders()
{
	shared_ptr<ID3DBlob> vsByteCode = m_device.CompileD3DShader(ShaderFile, "VS_Main", "vs_4_0");
	shared_ptr<ID3DBlob> psByteCode = m_device.CompileD3DShader(ShaderFile, "PS_Main", "ps_4_0");
	m_vertexShader = m_device.CreateVertexShader(vsByteCode);
	m_pixelShader = m_device.CreatePixelShader(psByteCode);
	m_inputLayout = m_device.CreateInputLayout<VertexPosNormal>(vsByteCode);

	vsByteCode = m_device.CompileD3DShader(ShaderFile, "VS_Bilboard", "vs_4_0");
	psByteCode = m_device.CompileD3DShader(ShaderFile, "PS_Bilboard", "ps_4_0");
	m_vertexShader2 = m_device.CreateVertexShader(vsByteCode);
	m_pixelShader2 = m_device.CreatePixelShader(psByteCode);
	m_inputLayout2 = m_device.CreateInputLayout<VertexPosNormal>(vsByteCode);
}

void Butterfly::InitializeConstantBuffers()
{
	D3D11_BUFFER_DESC desc;
	ZeroMemory(&desc, sizeof(D3D11_BUFFER_DESC));
	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	desc.ByteWidth = sizeof(XMMATRIX);
	desc.Usage = D3D11_USAGE_DEFAULT;
	m_cbWorld = m_device.CreateBuffer(desc);
	m_cbView = m_device.CreateBuffer(desc);
	m_cbProj = m_device.CreateBuffer(desc);
	desc.ByteWidth = sizeof(XMFLOAT4) * 3;
	m_cbLightPos = m_device.CreateBuffer(desc);
	desc.ByteWidth = sizeof(XMFLOAT4) * 5;
	m_cbLightColors = m_device.CreateBuffer(desc);
	desc.ByteWidth = sizeof(XMFLOAT4);
	m_cbSurfaceColor = m_device.CreateBuffer(desc);
}

void Butterfly::InitializeRenderStates()
//Setup render states used in various stages of the scene rendering
{
	D3D11_DEPTH_STENCIL_DESC dssDesc = m_device.DefaultDepthStencilDesc();
	dssDesc.StencilEnable = true;
	dssDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	dssDesc.BackFace.StencilFunc = D3D11_COMPARISON_NEVER;
	dssDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	dssDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
	m_dssWrite = m_device.CreateDepthStencilState(dssDesc);
	//Setup depth stencil state for writing
	dssDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dssDesc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
	dssDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	//Setup depth stencil state for testing
	m_dssTest = m_device.CreateDepthStencilState(dssDesc);

	D3D11_RASTERIZER_DESC rsDesc = m_device.DefaultRasterizerDesc();
	//Set rasterizer state front face to ccw
	m_rsCounterClockwise = m_device.CreateRasterizerState(rsDesc);

	D3D11_BLEND_DESC bsDesc = m_device.DefaultBlendDesc();

	bsDesc.RenderTarget[0].BlendEnable = TRUE;
	bsDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	bsDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	bsDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	bsDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	bsDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
	bsDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;

	//Setup alpha blending
	m_bsAlpha = m_device.CreateBlendState(bsDesc);

	D3D11_BLEND_DESC bsDesc2 = m_device.DefaultBlendDesc();

	bsDesc2.RenderTarget[0].BlendEnable = TRUE;
	bsDesc2.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_COLOR;
	bsDesc2.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_COLOR;
	bsDesc2.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	bsDesc2.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	bsDesc2.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
	bsDesc2.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;

	//Setup alpha blending
	m_bsBillboard = m_device.CreateBlendState(bsDesc2);
}

void Butterfly::InitializeCamera()
{
	SIZE s = getMainWindow()->getClientSize();
	float ar = static_cast<float>(s.cx) / s.cy;
	m_projMtx = XMMatrixPerspectiveFovLH(XM_PIDIV4, ar, 0.01f, 100.0f);
	m_context->UpdateSubresource(m_cbProj.get(), 0, 0, &m_projMtx, 0, 0);
	m_camera.Zoom(5);
	UpdateCamera();
}

void Butterfly::InitializeBox()
{
	VertexPosNormal vertices[] =
	{
		//Front face
		{ XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(0.5f, -0.5f, -0.5f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(0.5f, 0.5f, -0.5f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(-0.5f, 0.5f, -0.5f), XMFLOAT3(0.0f, 0.0f, 1.0f) },

		//Left face
		{ XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
		{ XMFLOAT3(-0.5f, 0.5f, -0.5f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
		{ XMFLOAT3(-0.5f, 0.5f, 0.5f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
		{ XMFLOAT3(-0.5f, -0.5f, 0.5f), XMFLOAT3(1.0f, 0.0f, 0.0f) },

		//Bottom face
		{ XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
		{ XMFLOAT3(-0.5f, -0.5f, 0.5f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
		{ XMFLOAT3(0.5f, -0.5f, 0.5f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
		{ XMFLOAT3(0.5f, -0.5f, -0.5f), XMFLOAT3(0.0f, 1.0f, 0.0f) },

		//Back face
		{ XMFLOAT3(-0.5f, -0.5f, 0.5f), XMFLOAT3(0.0f, 0.0f, -1.0f) },
		{ XMFLOAT3(-0.5f, 0.5f, 0.5f), XMFLOAT3(0.0f, 0.0f, -1.0f) },
		{ XMFLOAT3(0.5f, 0.5f, 0.5f), XMFLOAT3(0.0f, 0.0f, -1.0f) },
		{ XMFLOAT3(0.5f, -0.5f, 0.5f), XMFLOAT3(0.0f, 0.0f, -1.0f) },

		//Right face
		{ XMFLOAT3(0.5f, -0.5f, -0.5f), XMFLOAT3(-1.0f, 0.0f, 0.0f) },
		{ XMFLOAT3(0.5f, -0.5f, 0.5f), XMFLOAT3(-1.0f, 0.0f, 0.0f) },
		{ XMFLOAT3(0.5f, 0.5f, 0.5f), XMFLOAT3(-1.0f, 0.0f, 0.0f) },
		{ XMFLOAT3(0.5f, 0.5f, -0.5f), XMFLOAT3(-1.0f, 0.0f, 0.0f) },

		//Top face
		{ XMFLOAT3(-0.5f, 0.5f, -0.5f), XMFLOAT3(0.0f, -1.0f, 0.0f) },
		{ XMFLOAT3(0.5f, 0.5f, -0.5f), XMFLOAT3(0.0f, -1.0f, 0.0f) },
		{ XMFLOAT3(0.5f, 0.5f, 0.5f), XMFLOAT3(0.0f, -1.0f, 0.0f) },
		{ XMFLOAT3(-0.5f, 0.5f, 0.5f), XMFLOAT3(0.0f, -1.0f, 0.0f) },
	};
	m_vbBox = m_device.CreateVertexBuffer(vertices, 24);
	unsigned short indices[] =
	{
		0, 1, 2, 0, 2, 3,		//Front face
		4, 5, 6, 4, 6, 7,		//Left face
		8, 9, 10, 8, 10, 11,	//Botton face
		12, 13, 14, 12, 14, 15,	//Back face
		16, 17, 18, 16, 18, 19,	//Right face
		20, 21, 22, 20, 22, 23	//Top face
	};
	m_ibBox = m_device.CreateIndexBuffer(indices, 36);
}

void Butterfly::InitializePentagon()
{
	VertexPosNormal vertices[5];
	float a = 0, da = XM_2PI / 5.0f;
	for (int i = 0; i < 5; ++i, a -= da)
	{
		float sina, cosa;
		XMScalarSinCos(&sina, &cosa, a);
		vertices[i].Pos = XMFLOAT3(cosa, sina, 0.0f);
		vertices[i].Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
	}
	m_vbPentagon = m_device.CreateVertexBuffer(vertices, 5);
	unsigned short indices[] = { 0, 1, 2, 0, 2, 3, 0, 3, 4 };
	m_ibPentagon = m_device.CreateIndexBuffer(indices, 9);
}

void Butterfly::InitializeDodecahedron()
//Compute dodecahedronMtx and mirrorMtx
{
	m_dodecahedronMtx[0] = XMMatrixRotationX(XM_PIDIV2) * XMMatrixTranslation(0, -DODECAHEDRON_H * 0.5, 0);
	m_dodecahedronMtx[1] = m_dodecahedronMtx[0] * XMMatrixRotationZ(XM_PI - DODECAHEDRON_A) * XMMatrixRotationY(XM_PI);
	m_dodecahedronMtx[2] = m_dodecahedronMtx[1] * XMMatrixRotationY(XM_PI * 0.4);
	m_dodecahedronMtx[3] = m_dodecahedronMtx[2] * XMMatrixRotationY(XM_PI * 0.4);
	m_dodecahedronMtx[4] = m_dodecahedronMtx[3] * XMMatrixRotationY(XM_PI * 0.4);
	m_dodecahedronMtx[5] = m_dodecahedronMtx[4] * XMMatrixRotationY(XM_PI * 0.4);
	m_dodecahedronMtx[6] = m_dodecahedronMtx[0] * XMMatrixRotationZ(XM_PI);
	m_dodecahedronMtx[7] = m_dodecahedronMtx[6] * XMMatrixRotationZ(XM_PI - DODECAHEDRON_A) * XMMatrixRotationY(XM_PI);
	m_dodecahedronMtx[8] = m_dodecahedronMtx[7] * XMMatrixRotationY(XM_PI * 0.4);
	m_dodecahedronMtx[9] = m_dodecahedronMtx[8] * XMMatrixRotationY(XM_PI * 0.4);
	m_dodecahedronMtx[10] = m_dodecahedronMtx[9] * XMMatrixRotationY(XM_PI * 0.4);
	m_dodecahedronMtx[11] = m_dodecahedronMtx[10] * XMMatrixRotationY(XM_PI * 0.4);

	XMVECTOR * V = new XMVECTOR();
	for (size_t i = 0; i < 12; i++)
	{
		m_mirrorMtx[i] = XMMatrixInverse(V, m_dodecahedronMtx[i]) * XMMatrixScaling(1, 1, -1) * m_dodecahedronMtx[i];
	}
	delete V;
}

XMFLOAT3 Butterfly::MoebiusStripPos(float t, float s)
//Compute the position of point on the Moebius strip for parameters t and s
{
	float factor = MOEBIUS_R + MOEBIUS_W * s * XMScalarCos(0.5 * t);
	return XMFLOAT3(XMScalarCos(t) * factor, XMScalarSin(t) * factor, MOEBIUS_W * s * sin(0.5 * t));
}

XMVECTOR Butterfly::MoebiusStripDt(float t, float s)
//Compute the t-derivative of point on the Moebius strip for parameters t and s
{
	float f1 = -MOEBIUS_R * XMScalarSin(t) - 0.5 * s * MOEBIUS_W * XMScalarSin(0.5 * t) * XMScalarCos(t) - MOEBIUS_W * s
		* XMScalarCos(0.5 * t) * XMScalarSin(t);
	float f2 = MOEBIUS_R * XMScalarCos(t) - 0.5 * s * MOEBIUS_W * XMScalarSin(0.5 * t) * XMScalarSin(t) + MOEBIUS_W * s
		* XMScalarCos(0.5 * t) * XMScalarCos(t);
	float f3 = 0.5 * s * MOEBIUS_W * XMScalarCos(t);
	XMFLOAT3 dt(f1, f2, f3);
	return XMLoadFloat3(&dt);
}

XMVECTOR Butterfly::MoebiusStripDs(float t, float s)
// Return the s-derivative of point on the Moebius strip for parameters t and s
{
	XMFLOAT3 dt(XMScalarCos(0.5 * t) * XMScalarCos(t), XMScalarCos(0.5 * t) * XMScalarSin(t), XMScalarSin(0.5 * t));
	return XMLoadFloat3(&dt);
}

XMMATRIX Butterfly::MoebiusStripMatrix(float t, float s)
{
	XMVECTOR nPs = XMVector3Normalize(MoebiusStripDs(t, s));
	XMVECTOR nPt = XMVector3Normalize(MoebiusStripDt(t, s));
	XMVECTOR nPst = XMVector3Normalize(XMVector3Cross(nPs, nPt));
	XMVECTOR p = XMLoadFloat3(&MoebiusStripPos(t, s));
	nPs.m128_f32[3] = 0;
	nPt.m128_f32[3] = 0;
	nPst.m128_f32[3] = 0;
	p.m128_f32[3] = 1;
	/*XMMATRIX result;
	result.r[0] = */
	return XMMATRIX(nPs, nPt, nPst, p);
}

XMFLOAT3 Butterfly::MoebiusStripNorm(float t, float s)
{
	XMVECTOR nPs = XMVector3Normalize(MoebiusStripDs(t, s));
	XMVECTOR nPt = XMVector3Normalize(MoebiusStripDt(t, s));
	XMVECTOR nPts = XMVector3Normalize(XMVector3Cross(nPs, nPt));
	XMFLOAT3 result;
	XMStoreFloat3(&result, nPts);
	return result;
}

void Butterfly::InitializeMoebiusStrip()
//Create vertex and index buffers for the Moebius strip
{
	float dst = XM_PI * 4 / MOEBIUS_N;
	VertexPosNormal vertices[MOEBIUS_N * 2];
	short idx = 0;
	float t = 0;
	for (size_t i = 0; i < MOEBIUS_N; i++)
	{
		vertices[idx].Pos = MoebiusStripPos(t, -1);
		vertices[idx++].Normal = MoebiusStripNorm(t, -1);
		vertices[idx].Pos = MoebiusStripPos(t, 1);
		vertices[idx++].Normal = MoebiusStripNorm(t, 1);
		t += dst;
	}
	m_vbMoebius = m_device.CreateVertexBuffer(vertices, MOEBIUS_N * 2);

	unsigned short indices[MOEBIUS_N * 6];
	idx = 0;
	for (size_t i = 0; i < MOEBIUS_N; i++)
	{
		indices[idx++] = (i * 2) % (MOEBIUS_N * 2);
		indices[idx++] = (i * 2 + 1) % (MOEBIUS_N * 2);
		indices[idx++] = (i * 2 + 2) % (MOEBIUS_N * 2);
	}
	for (size_t i = 0; i < MOEBIUS_N; i++)
	{
		indices[idx++] = (i * 2 + 1) % (MOEBIUS_N * 2);
		indices[idx++] = (i * 2 + 3) % (MOEBIUS_N * 2);
		indices[idx++] = (i * 2 + 2) % (MOEBIUS_N * 2);
	}
	m_ibMoebius = m_device.CreateIndexBuffer(indices, MOEBIUS_N * 6);
}

void Butterfly::InitializeButterfly()
//Create vertex and index buffers for the butterfly wing
{
	VertexPosNormal vertices[8];
	vertices[0].Pos = XMFLOAT3(-1, 1, 0);
	vertices[1].Pos = XMFLOAT3(1, 1, 0);
	vertices[2].Pos = XMFLOAT3(-1, -1, 0);
	vertices[3].Pos = XMFLOAT3(1, -1, 0);
	vertices[4].Pos = XMFLOAT3(-1, 1, 0);
	vertices[5].Pos = XMFLOAT3(1, 1, 0);
	vertices[6].Pos = XMFLOAT3(-1, -1, 0);
	vertices[7].Pos = XMFLOAT3(1, -1, 0);
	for (size_t i = 0; i < 8; i++)
	{
		vertices[i].Normal = XMFLOAT3(0, 0, (i % 2)* -2 + 1);
	}
	unsigned short indices[]
	{
		0, 1, 2,
			1, 3, 2,
			4, 6, 5,
			5, 6, 7
	};
	m_vbWing = m_device.CreateVertexBuffer(vertices, 8);
	m_ibWing = m_device.CreateIndexBuffer(indices, 12);

}

void Butterfly::InitializeBilboards()
//Initialize bilboard resources (vertex, pixel shaders, input layout, vertex, index buffers etc.)
{
	VertexPos vertices[4];
	vertices[0].Pos = XMFLOAT3(-1, 1, 0);
	vertices[1].Pos = XMFLOAT3(1, 1, 0);
	vertices[2].Pos = XMFLOAT3(-1, -1, 0);
	vertices[3].Pos = XMFLOAT3(1, -1, 0);
	unsigned short indices[]
	{
		0, 1, 2,
			1, 3, 2
	};
	m_vbBillboard = m_device.CreateVertexBuffer(vertices, 4);
	m_ibBillboard = m_device.CreateIndexBuffer(indices, 6);
	XMFLOAT4 cameraPosition = m_camera.GetPosition();
	XMMATRIX invView = m_camera.GetViewMatrix();
	XMVECTOR * vector = new XMVECTOR();
	invView = XMMatrixInverse(vector, invView);
	delete(vector);
	m_billboardMtx = XMMatrixTranslation(0, -DODECAHEDRON_H * 0.5, 0) * XMMatrixScaling(0.5, 0.5, 0.5) * XMMatrixTranslation(-cameraPosition.x, -cameraPosition.y, -cameraPosition.z) * invView;
}

void Butterfly::SetShaders()
{
	m_context->IASetInputLayout(m_inputLayout.get());
	m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_context->VSSetShader(m_vertexShader.get(), 0, 0);
	m_context->PSSetShader(m_pixelShader.get(), 0, 0);
}

void Butterfly::SetBillboardShaders()
{
	m_context->IASetInputLayout(m_inputLayout2.get());
	m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_context->VSSetShader(m_vertexShader2.get(), 0, 0);
	m_context->PSSetShader(m_pixelShader2.get(), 0, 0);
}

void Butterfly::SetConstantBuffers()
{
	ID3D11Buffer* vsb[] = { m_cbWorld.get(), m_cbView.get(), m_cbProj.get(), m_cbLightPos.get() };
	m_context->VSSetConstantBuffers(0, 4, vsb);
	ID3D11Buffer* psb[] = { m_cbLightColors.get(), m_cbSurfaceColor.get() };
	m_context->PSSetConstantBuffers(0, 2, psb);
}

bool Butterfly::LoadContent()
{
	InitializeShaders();
	InitializeConstantBuffers();
	InitializeRenderStates();
	InitializeCamera();
	InitializeBox();
	InitializePentagon();
	InitializeDodecahedron();
	InitializeMoebiusStrip();
	InitializeButterfly();
	InitializeBilboards();

	SetShaders();
	SetConstantBuffers();

	return true;
}

void Butterfly::UnloadContent()
{
	m_vertexShader.reset();
	m_pixelShader.reset();
	m_inputLayout.reset();

	m_vertexShader2.reset();
	m_pixelShader2.reset();
	m_inputLayout2.reset();

	m_dssWrite.reset();
	m_dssTest.reset();
	m_rsCounterClockwise.reset();
	m_bsAlpha.reset();

	m_vbBox.reset();
	m_ibBox.reset();
	m_vbPentagon.reset();
	m_ibPentagon.reset();
	m_vbMoebius.reset();
	m_ibMoebius.reset();

	m_cbWorld.reset();
	m_cbView.reset();
	m_cbProj.reset();
	m_cbLightPos.reset();
	m_cbLightColors.reset();
	m_cbSurfaceColor.reset();
}

void Butterfly::UpdateCamera()
{
	XMMATRIX viewMtx;
	m_camera.GetViewMatrix(viewMtx);
	m_context->UpdateSubresource(m_cbView.get(), 0, 0, &viewMtx, 0, 0);
}

void Butterfly::UpdateCamera(XMMATRIX &matrix)
{
	m_context->UpdateSubresource(m_cbView.get(), 0, 0, &matrix, 0, 0);
}

void Butterfly::UpdateButterfly(float dtime)
//Compute the matrices for butterfly wings. Position on the strip is determined based on time
{
	//Time passed since the current lap started
	static float lap = 0.0f;
	lap += dtime;
	while (lap > LAP_TIME)
		lap -= LAP_TIME;
	//Value of the Moebius strip t parameter
	float t = 2 * lap / LAP_TIME;
	//Angle between wing current and vertical position
	float a = t * WING_MAX_A;
	t *= XM_2PI;
	if (a > WING_MAX_A)
		a = 2 * WING_MAX_A - a;

	m_wingMtx[0] = XMMatrixTranslation(1, 0, 0) * XMMatrixScaling(WING_H * 0.5, WING_W * 0.5, 1) * XMMatrixRotationY(XM_PI * 0.5 + a) * MoebiusStripMatrix(t, 0);
	m_wingMtx[1] = XMMatrixTranslation(1, 0, 0) * XMMatrixScaling(WING_H * 0.5, WING_W * 0.5, 1) * XMMatrixRotationY(XM_PI * 0.5 - a) * MoebiusStripMatrix(t, 0);
}

void Butterfly::SetLight0()
//Setup one positional light at the camera
{
	XMFLOAT4 positions[3];
	ZeroMemory(positions, sizeof(XMFLOAT4) * 3);
	positions[0] = m_camera.GetPosition();
	m_context->UpdateSubresource(m_cbLightPos.get(), 0, 0, positions, 0, 0);

	XMFLOAT4 colors[5];
	ZeroMemory(colors, sizeof(XMFLOAT4) * 5);
	colors[0] = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f); //ambient color
	colors[1] = XMFLOAT4(1.0f, 0.8f, 1.0f, 100.0f); //surface [ka, kd, ks, m]
	colors[2] = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f); //light0 color
	m_context->UpdateSubresource(m_cbLightColors.get(), 0, 0, colors, 0, 0);
}

void Butterfly::SetLight1()
//Setup one white positional light at the camera
//Setup two additional positional lights, green and blue.
{
	XMFLOAT4 positions[3];
	ZeroMemory(positions, sizeof(XMFLOAT4) * 3);
	positions[0] = m_camera.GetPosition(); //white light position
	positions[1] = BLUE_LIGHT_POS; //blue light position
	positions[2] = GREEN_LIGHT_POS; //green light position
	m_context->UpdateSubresource(m_cbLightPos.get(), 0, 0, positions, 0, 0);

	XMFLOAT4 colors[5];
	ZeroMemory(colors, sizeof(XMFLOAT4) * 5);
	colors[0] = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f); //ambient color
	colors[1] = XMFLOAT4(1.0f, 0.8f, 1.0f, 100.0f); //surface [ka, kd, ks, m]
	colors[2] = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f); //white light color
	colors[3] = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f); //blue light color
	colors[4] = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f); //green light color
	m_context->UpdateSubresource(m_cbLightColors.get(), 0, 0, colors, 0, 0);
}

void Butterfly::SetSurfaceColor(const XMFLOAT4& color)
{
	m_context->UpdateSubresource(m_cbSurfaceColor.get(), 0, 0, &color, 0, 0);
}

void Butterfly::Update(float dt)
{
	UpdateButterfly(dt);
	static MouseState prevState;
	MouseState currentState;
	if (!m_mouse->GetState(currentState))
		return;
	bool change = true;
	if (prevState.isButtonDown(0))
	{
		POINT d = currentState.getMousePositionChange();
		m_camera.Rotate(d.y / 300.f, d.x / 300.f);
	}
	else if (prevState.isButtonDown(1))
	{
		POINT d = currentState.getMousePositionChange();
		m_camera.Zoom(d.y / 10.0f);
	}
	else
		change = false;
	prevState = currentState;
	if (change)
		UpdateCamera();
}

void Butterfly::DrawBox()
{
	const XMMATRIX worldMtx = XMMatrixIdentity();
	m_context->UpdateSubresource(m_cbWorld.get(), 0, 0, &worldMtx, 0, 0);

	ID3D11Buffer* b = m_vbBox.get();
	m_context->IASetVertexBuffers(0, 1, &b, &VB_STRIDE, &VB_OFFSET);
	m_context->IASetIndexBuffer(m_ibBox.get(), DXGI_FORMAT_R16_UINT, 0);
	m_context->DrawIndexed(36, 0, 0);
}

void Butterfly::DrawDodecahedron(bool colors)
//Draw dodecahedron. If color is true, use render faces with coresponding colors. Otherwise render using white color
{
	for (size_t i = 0; i < 12; i++)
	{
		const XMMATRIX worldMtx = XMMatrixIdentity() * m_dodecahedronMtx[i];
		m_context->UpdateSubresource(m_cbWorld.get(), 0, 0, &worldMtx, 0, 0);
		if (colors)
			m_context->UpdateSubresource(m_cbSurfaceColor.get(), 0, 0, &COLORS[i], 0, 0);

		ID3D11Buffer* b = m_vbPentagon.get();
		m_context->IASetVertexBuffers(0, 1, &b, &VB_STRIDE, &VB_OFFSET);
		m_context->IASetIndexBuffer(m_ibPentagon.get(), DXGI_FORMAT_R16_UINT, 0);
		m_context->DrawIndexed(9, 0, 0);
	}
}

void Butterfly::DrawMoebiusStrip()
//Draw the Moebius strip
{
	const XMMATRIX worldMtx = XMMatrixIdentity();
	XMFLOAT4 color{ 1.0f, 1.0f, 1.0f, 1.0f };
	m_context->UpdateSubresource(m_cbSurfaceColor.get(), 0, 0, &color, 0, 0);
	m_context->UpdateSubresource(m_cbWorld.get(), 0, 0, &worldMtx, 0, 0);
	ID3D11Buffer* b = m_vbMoebius.get();
	m_context->IASetVertexBuffers(0, 1, &b, &VB_STRIDE, &VB_OFFSET);
	m_context->IASetIndexBuffer(m_ibMoebius.get(), DXGI_FORMAT_R16_UINT, 0);
	m_context->DrawIndexed(MOEBIUS_N * 6, 0, 0);
}

void Butterfly::DrawButterfly()
//Draw the butterfly
{
	for (size_t i = 0; i < 2; i++)
	{
		const XMMATRIX worldMtx = XMMatrixIdentity() * m_wingMtx[i];
		m_context->UpdateSubresource(m_cbWorld.get(), 0, 0, &worldMtx, 0, 0);
		ID3D11Buffer* b = m_vbWing.get();
		m_context->IASetVertexBuffers(0, 1, &b, &VB_STRIDE, &VB_OFFSET);
		m_context->IASetIndexBuffer(m_ibWing.get(), DXGI_FORMAT_R16_UINT, 0);
		m_context->DrawIndexed(12, 0, 0);
	}
}

void Butterfly::DrawBilboards()
//Setup bilboards rendering and draw them
{
	m_context->OMSetBlendState(m_bsBillboard.get(), 0, BS_MASK);
	SetBillboardShaders();

	const XMMATRIX worldMtx = XMMatrixIdentity() * m_billboardMtx;
	m_context->UpdateSubresource(m_cbWorld.get(), 0, 0, &worldMtx, 0, 0);
	ID3D11Buffer* b = m_vbBillboard.get();
	m_context->IASetVertexBuffers(0, 1, &b, &VB_STRIDE, &VB_OFFSET);
	m_context->IASetIndexBuffer(m_ibBillboard.get(), DXGI_FORMAT_R16_UINT, 0);
	m_context->DrawIndexed(6, 0, 0);

	SetShaders();
	m_context->OMSetBlendState(0, 0, BS_MASK);
}

void Butterfly::DrawMirroredWorld(int i)
//Draw the mirrored scene reflected in the i-th dodecahedron face
{
	//Setup render state for writing to the stencil buffer

	//Draw the i-th face

	//Setup render state and view matrix for rendering the mirrored world

	//Draw objects

	//Restore rendering state to it's original values

	const XMMATRIX worldMtx = XMMatrixIdentity() * m_dodecahedronMtx[i];
	m_context->UpdateSubresource(m_cbWorld.get(), 0, 0, &worldMtx, 0, 0);
	ID3D11Buffer* b = m_vbPentagon.get();
	m_context->IASetVertexBuffers(0, 1, &b, &VB_STRIDE, &VB_OFFSET);
	m_context->IASetIndexBuffer(m_ibPentagon.get(), DXGI_FORMAT_R16_UINT, 0);
	m_context->OMSetDepthStencilState(m_dssWrite.get(), i + 1);
	m_context->DrawIndexed(9, 0, 0);
	m_context->OMSetDepthStencilState(m_dssTest.get(), i + 1);
	m_context->RSSetState(m_rsCounterClockwise.get());
	UpdateCamera(m_mirrorMtx[i] * m_camera.GetViewMatrix());
	DrawMoebiusStrip();
	DrawButterfly();
	SetLight0();
	DrawDodecahedron(false);
	SetLight1();
	m_context->RSSetState(NULL);
	m_context->OMSetDepthStencilState(NULL, 0);
	UpdateCamera();
}

void Butterfly::Render()
{
	if (m_context == nullptr)
		return;
	//Clear buffers
	float clearColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	m_context->ClearRenderTargetView(m_backBuffer.get(), clearColor);
	m_context->ClearDepthStencilView(m_depthStencilView.get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	//Render box with all three lights
	SetLight1();
	SetSurfaceColor(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	//DrawBox();

	//render mirrored worlds
	for (int i = 0; i < 12; ++i)
		DrawMirroredWorld(i);

	//render dodecahedron with one light and alpha blending
	m_context->OMSetBlendState(m_bsAlpha.get(), 0, BS_MASK);
	SetLight0();
	DrawDodecahedron(true);
	m_context->OMSetBlendState(0, 0, BS_MASK);

	//render the rest of the scene with all lights
	SetLight1();
	DrawMoebiusStrip();
	DrawButterfly();
	DrawBilboards();

	m_swapChain->Present(0, 0);
}