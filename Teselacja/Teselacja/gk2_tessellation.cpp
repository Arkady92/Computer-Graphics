#include "gk2_tessellation.h"
#include "gk2_window.h"
#include "gk2_vertices.h"

using namespace gk2;
using namespace std;

Tessellation::Tessellation(HINSTANCE hInstance)
	: ApplicationBase(hInstance), m_camera(0.01f, 100.0f)
{

}

Tessellation::~Tessellation()
{

}

void * Tessellation::operator new(size_t size)
{
	return Utils::New16Aligned(size);
}

void Tessellation::operator delete(void * ptr)
{
	Utils::Delete16Aligned(ptr);
}

void Tessellation::InitializeConstantBuffers()
{
	m_projCB.reset(new CBMatrix(m_device));
	m_viewCB.reset(new CBMatrix(m_device));
	m_worldCB.reset(new CBMatrix(m_device));
	m_surfaceColorCB.reset(new ConstantBuffer<XMFLOAT4>(m_device));
	m_cameraPosCB.reset(new ConstantBuffer<XMFLOAT4>(m_device));
	m_patchIndexCB.reset(new ConstantBuffer<INT>(m_device));
	m_edgeTessellationFactorCB.reset(new ConstantBuffer<FLOAT>(m_device));
	m_interiorTessellationFactorCB.reset(new ConstantBuffer<FLOAT>(m_device));
}

void Tessellation::InitializeCamera()
{
	SIZE s = getMainWindow()->getClientSize();
	float ar = static_cast<float>(s.cx) / s.cy;
	m_projMtx = XMMatrixPerspectiveFovLH(XM_PIDIV4, ar, 0.01f, 100.0f);
	m_projCB->Update(m_context, m_projMtx);
	m_camera.Zoom(5);
	UpdateCamera();
}

void Tessellation::InitializeRenderStates()
{
	D3D11_RASTERIZER_DESC rsDesc = m_device.DefaultRasterizerDesc();
	rsDesc.CullMode = D3D11_CULL_NONE;
	rsDesc.FillMode = D3D11_FILL_WIREFRAME;
	m_rsWireframe = m_device.CreateRasterizerState(rsDesc);

	rsDesc.FillMode = D3D11_FILL_SOLID;
	m_rsSolid = m_device.CreateRasterizerState(rsDesc);

	D3D11_SAMPLER_DESC sd = m_device.DefaultSamplerDesc();
	sd.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sd.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	m_domainSamplerWrap = m_device.CreateSamplerState(sd);
	m_pixelSamplerWrap = m_device.CreateSamplerState(sd);
}

void Tessellation::InitializeTextures()
{
	m_displacementTexture = m_device.CreateShaderResourceView(L"resources/textures/height.dds");
	m_colorTexture = m_device.CreateShaderResourceView(L"resources/textures/diffuse.dds");
	m_normalTexture = m_device.CreateShaderResourceView(L"resources/textures/normals.dds");
}

bool Tessellation::LoadContent()
{
	InitializeConstantBuffers();
	InitializeRenderStates();
	InitializeTextures();
	InitializeCamera();

	part = parts::I;
	m_edgeTessellationFactorCB->Update(m_context, ETF);
	m_interiorTessellationFactorCB->Update(m_context, ITF);

	m_colorEffect.reset(new ColorEffect(m_device, m_layout));
	m_colorEffect->SetProjMtxBuffer(m_projCB);
	m_colorEffect->SetViewMtxBuffer(m_viewCB);
	m_colorEffect->SetWorldMtxBuffer(m_worldCB);
	m_colorEffect->SetSurfaceColorBuffer(m_surfaceColorCB);

	m_partIEffect.reset(new PartIEffect(m_device, m_layout));
	m_partIEffect->SetProjMtxBuffer(m_projCB);
	m_partIEffect->SetViewMtxBuffer(m_viewCB);
	m_partIEffect->SetWorldMtxBuffer(m_worldCB);
	m_partIEffect->SetSurfaceColorBuffer(m_surfaceColorCB);
	m_partIEffect->SetEedgeTessellationFactorBuffer(m_edgeTessellationFactorCB);
	m_partIEffect->SetInteriorTessellationFactorBuffer(m_interiorTessellationFactorCB);

	m_partIIEffect.reset(new PartIIEffect(m_device, m_layout));
	m_partIIEffect->SetProjMtxBuffer(m_projCB);
	m_partIIEffect->SetViewMtxBuffer(m_viewCB);
	m_partIIEffect->SetWorldMtxBuffer(m_worldCB);
	m_partIIEffect->SetSurfaceColorBuffer(m_surfaceColorCB);
	m_partIIEffect->SetEedgeTessellationFactorBuffer(m_edgeTessellationFactorCB);
	m_partIIEffect->SetInteriorTessellationFactorBuffer(m_interiorTessellationFactorCB);

	m_partIIIEffect.reset(new PartIIIEffect(m_device, m_layout));
	m_partIIIEffect->SetProjMtxBuffer(m_projCB);
	m_partIIIEffect->SetViewMtxBuffer(m_viewCB);
	m_partIIIEffect->SetWorldMtxBuffer(m_worldCB);
	m_partIIIEffect->SetSurfaceColorBuffer(m_surfaceColorCB);
	m_partIIIEffect->SetEedgeTessellationFactorBuffer(m_edgeTessellationFactorCB);
	m_partIIIEffect->SetInteriorTessellationFactorBuffer(m_interiorTessellationFactorCB);

	m_partIVVEffect.reset(new PartIVVEffect(m_device, m_layout));
	m_partIVVEffect->SetProjMtxBuffer(m_projCB);
	m_partIVVEffect->SetViewMtxBuffer(m_viewCB);
	m_partIVVEffect->SetWorldMtxBuffer(m_worldCB);
	m_partIVVEffect->SetSurfaceColorBuffer(m_surfaceColorCB);
	m_partIVVEffect->SetIndexBuffer(m_patchIndexCB);
	m_partIVVEffect->SetEedgeTessellationFactorBuffer(m_edgeTessellationFactorCB);
	m_partIVVEffect->SetInteriorTessellationFactorBuffer(m_interiorTessellationFactorCB);
	m_partIVVEffect->SetCameraPosBuffer(m_cameraPosCB);
	m_partIVVEffect->SetSamplerState(m_domainSamplerWrap, m_pixelSamplerWrap);
	m_partIVVEffect->SetDisplacementTexture(m_displacementTexture);
	m_partIVVEffect->SetColorTexture(m_colorTexture);
	m_partIVVEffect->SetNormalTexture(m_normalTexture);

	InitializeQuadVertexBuffer();
	InitializeBezierControlNetVertexBuffers();
	InitializeBezierControlNetIndicesBuffer();
	return true;
}

void Tessellation::InitializeQuadVertexBuffer()
{
	VertexPos vertices[4] =
	{
		{ XMFLOAT3(-1.0f, -1.0f, 0.0f) },
		{ XMFLOAT3(1.0f, -1.0f, 0.0f) },
		{ XMFLOAT3(1.0f, 1.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, 0.0f) }
	};
	m_quadVertexBuffer = m_device.CreateVertexBuffer(vertices, m_quadVertexCount);
	m_vertexStride = sizeof(VertexPos);
}

void Tessellation::InitializeBezierControlNetVertexBuffers()
{
	VertexPos holeBezierPatch[16] =
	{
		{ XMFLOAT3(-1.0f, 1.0f, 0.0f) },
		{ XMFLOAT3(-1.0f / 3.0f, 1.0f, 0.0f) },
		{ XMFLOAT3(1.0f / 3.0f, 1.0f, 0.0f) },
		{ XMFLOAT3(1.0f, 1.0f, 0.0f) },

		{ XMFLOAT3(-1.0f, 1.0f / 3.0f, 0.0f) },
		{ XMFLOAT3(-1.0f / 3.0f, 1.0f / 3.0f, 1.0f) },
		{ XMFLOAT3(1.0f / 3.0f, 1.0f / 3.0f, 1.0f) },
		{ XMFLOAT3(1.0f, 1.0f / 3.0f, 0.0f) },

		{ XMFLOAT3(-1.0f, -1.0f / 3.0f, 0.0f) },
		{ XMFLOAT3(-1.0f / 3.0f, -1.0f / 3.0f, 1.0f) },
		{ XMFLOAT3(1.0f / 3.0f, -1.0f / 3.0f, 1.0f) },
		{ XMFLOAT3(1.0f, -1.0f / 3.0f, .0f) },

		{ XMFLOAT3(-1.0f, -1.0f, 0.0f) },
		{ XMFLOAT3(-1.0f / 3.0f, -1.0f, 0.0f) },
		{ XMFLOAT3(1.0f / 3.0f, -1.0f, 0.0f) },
		{ XMFLOAT3(1.0f, -1.0f, 0.0f) },
	};

	m_bezierPatchVertexBuffers[0] = m_device.CreateVertexBuffer(holeBezierPatch, m_bezierPatchVertexCount);
	m_bezierControlNetVertexBuffers[0] = m_device.CreateVertexBuffer(holeBezierPatch, m_bezierPatchVertexCount);

	VertexPos randomBezierPatch[16] =
	{
		{ XMFLOAT3(-1.0f, 1.0f, (rand() % 2000 - 1000) / 1000.0f) },
		{ XMFLOAT3(-1.0f / 3.0f, 1.0f, (rand() % 2000 - 1000) / 1000.0f) },
		{ XMFLOAT3(1.0f / 3.0f, 1.0f, (rand() % 2000 - 1000) / 1000.0f) },
		{ XMFLOAT3(1.0f, 1.0f, (rand() % 2000 - 1000) / 1000.0f) },

		{ XMFLOAT3(-1.0f, 1.0f / 3.0f, (rand() % 2000 - 1000) / 1000.0f) },
		{ XMFLOAT3(-1.0f / 3.0f, 1.0f / 3.0f, (rand() % 2000 - 1000) / 1000.0f) },
		{ XMFLOAT3(1.0f / 3.0f, 1.0f / 3.0f, (rand() % 2000 - 1000) / 1000.0f) },
		{ XMFLOAT3(1.0f, 1.0f / 3.0f, (rand() % 2000 - 1000) / 1000.0f) },

		{ XMFLOAT3(-1.0f, -1.0f / 3.0f, (rand() % 2000 - 1000) / 1000.0f) },
		{ XMFLOAT3(-1.0f / 3.0f, -1.0f / 3.0f, (rand() % 2000 - 1000) / 1000.0f) },
		{ XMFLOAT3(1.0f / 3.0f, -1.0f / 3.0f, (rand() % 2000 - 1000) / 1000.0f) },
		{ XMFLOAT3(1.0f, -1.0f / 3.0f, (rand() % 2000 - 1000) / 1000.0f) },

		{ XMFLOAT3(-1.0f, -1.0f, (rand() % 2000 - 1000) / 1000.0f) },
		{ XMFLOAT3(-1.0f / 3.0f, -1.0f, (rand() % 2000 - 1000) / 1000.0f) },
		{ XMFLOAT3(1.0f / 3.0f, -1.0f, (rand() % 2000 - 1000) / 1000.0f) },
		{ XMFLOAT3(1.0f, -1.0f, (rand() % 2000 - 1000) / 1000.0f) },
	};

	m_bezierPatchVertexBuffers[1] = m_device.CreateVertexBuffer(randomBezierPatch, m_bezierPatchVertexCount);
	m_bezierControlNetVertexBuffers[1] = m_device.CreateVertexBuffer(randomBezierPatch, m_bezierPatchVertexCount);

	VertexPos drainBezierPatch[16] =
	{
		{ XMFLOAT3(-2.0f, 2.0f, 0.0f) },
		{ XMFLOAT3(-2.0f / 3.0f, 2.0f, 0.0f) },
		{ XMFLOAT3(2.0f / 3.0f, 2.0f, 0.0f) },
		{ XMFLOAT3(2.0f, 2.0f, 0.0f) },

		{ XMFLOAT3(-2.0f, 2.0f / 3.0f, 2.0f) },
		{ XMFLOAT3(-2.0f / 3.0f, 2.0f / 3.0f, 2.0f) },
		{ XMFLOAT3(2.0f / 3.0f, 2.0f / 3.0f, 2.0f) },
		{ XMFLOAT3(2.0f, 2.0f / 3.0f, 2.0f) },

		{ XMFLOAT3(-2.0f, -2.0f / 3.0f, 2.0f) },
		{ XMFLOAT3(-2.0f / 3.0f, -2.0f / 3.0f, 2.0f) },
		{ XMFLOAT3(2.0f / 3.0f, -2.0f / 3.0f, 2.0f) },
		{ XMFLOAT3(2.0f, -2.0f / 3.0f, 2.0f) },

		{ XMFLOAT3(-2.0f, -2.0f, 0.0f) },
		{ XMFLOAT3(-2.0f / 3.0f, -2.0f, 0.0f) },
		{ XMFLOAT3(2.0f / 3.0f, -2.0f, 0.0f) },
		{ XMFLOAT3(2.0f, -2.0f, 0.0f) },
	};

	float L = 2.0f;
	float H = 6.0f;
	VertexPos multipleBezierPatchVertexBuffers[16][16];
	for (size_t i = 0; i < 16; i++)
	{
		multipleBezierPatchVertexBuffers[0][i].Pos = XMFLOAT3(drainBezierPatch[i].Pos.x - H, drainBezierPatch[i].Pos.y - H, drainBezierPatch[i].Pos.z);
		multipleBezierPatchVertexBuffers[1][i].Pos = XMFLOAT3(drainBezierPatch[i].Pos.x - L, drainBezierPatch[i].Pos.y - H, drainBezierPatch[i].Pos.z);
		multipleBezierPatchVertexBuffers[2][i].Pos = XMFLOAT3(drainBezierPatch[i].Pos.x + L, drainBezierPatch[i].Pos.y - H, drainBezierPatch[i].Pos.z);
		multipleBezierPatchVertexBuffers[3][i].Pos = XMFLOAT3(drainBezierPatch[i].Pos.x + H, drainBezierPatch[i].Pos.y - H, drainBezierPatch[i].Pos.z);
		multipleBezierPatchVertexBuffers[4][i].Pos = XMFLOAT3(drainBezierPatch[i].Pos.x - H, drainBezierPatch[i].Pos.y - L, -drainBezierPatch[i].Pos.z);
		multipleBezierPatchVertexBuffers[5][i].Pos = XMFLOAT3(drainBezierPatch[i].Pos.x - L, drainBezierPatch[i].Pos.y - L, -drainBezierPatch[i].Pos.z);
		multipleBezierPatchVertexBuffers[6][i].Pos = XMFLOAT3(drainBezierPatch[i].Pos.x + L, drainBezierPatch[i].Pos.y - L, -drainBezierPatch[i].Pos.z);
		multipleBezierPatchVertexBuffers[7][i].Pos = XMFLOAT3(drainBezierPatch[i].Pos.x + H, drainBezierPatch[i].Pos.y - L, -drainBezierPatch[i].Pos.z);
		multipleBezierPatchVertexBuffers[8][i].Pos = XMFLOAT3(drainBezierPatch[i].Pos.x - H, drainBezierPatch[i].Pos.y + L, drainBezierPatch[i].Pos.z);
		multipleBezierPatchVertexBuffers[9][i].Pos = XMFLOAT3(drainBezierPatch[i].Pos.x - L, drainBezierPatch[i].Pos.y + L, drainBezierPatch[i].Pos.z);
		multipleBezierPatchVertexBuffers[10][i].Pos = XMFLOAT3(drainBezierPatch[i].Pos.x + L, drainBezierPatch[i].Pos.y + L, drainBezierPatch[i].Pos.z);
		multipleBezierPatchVertexBuffers[11][i].Pos = XMFLOAT3(drainBezierPatch[i].Pos.x + H, drainBezierPatch[i].Pos.y + L, drainBezierPatch[i].Pos.z);
		multipleBezierPatchVertexBuffers[12][i].Pos = XMFLOAT3(drainBezierPatch[i].Pos.x - H, drainBezierPatch[i].Pos.y + H, -drainBezierPatch[i].Pos.z);
		multipleBezierPatchVertexBuffers[13][i].Pos = XMFLOAT3(drainBezierPatch[i].Pos.x - L, drainBezierPatch[i].Pos.y + H, -drainBezierPatch[i].Pos.z);
		multipleBezierPatchVertexBuffers[14][i].Pos = XMFLOAT3(drainBezierPatch[i].Pos.x + L, drainBezierPatch[i].Pos.y + H, -drainBezierPatch[i].Pos.z);
		multipleBezierPatchVertexBuffers[15][i].Pos = XMFLOAT3(drainBezierPatch[i].Pos.x + H, drainBezierPatch[i].Pos.y + H, -drainBezierPatch[i].Pos.z);
	}

	for (size_t i = 0; i < 16; i++)
		m_multipleBezierPatchVertexBuffers[i] = (m_device.CreateVertexBuffer(multipleBezierPatchVertexBuffers[i], m_bezierPatchVertexCount));
}

void Tessellation::InitializeBezierControlNetIndicesBuffer()
{
	vector<unsigned short> indices;
	for (size_t i = 0; i < 4; i++)
	{
		indices.push_back(i * 4);
		indices.push_back((i * 4) + 1);
		indices.push_back((i * 4) + 1);
		indices.push_back((i * 4) + 2);
		indices.push_back((i * 4) + 2);
		indices.push_back((i * 4) + 3);

		indices.push_back(i);
		indices.push_back(i + 4);
		indices.push_back(i + 4);
		indices.push_back(i + 8);
		indices.push_back(i + 8);
		indices.push_back(i + 12);
	}

	m_bezierControlNetIndexBuffer = m_device.CreateIndexBuffer(&(indices[0]), indices.size());
}

void Tessellation::UnloadContent()
{

}

void Tessellation::UpdateCamera()
{
	XMMATRIX view;
	m_camera.GetViewMatrix(view);
	m_viewCB->Update(m_context, view);
	m_cameraPosCB->Update(m_context, m_camera.GetPosition());
}

void Tessellation::Update(float dt)
{
	static MouseState prevState;
	static KeyboardState prevKeyState;

	MouseState currentState;
	if (m_mouse->GetState(currentState))
	{
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

	KeyboardState currentKeyState;
	float zoomValue = 32.0f;
	if (m_keyboard->GetState(currentKeyState))
	{
		if (prevKeyState.isKeyDown(DIK_RIGHT) && currentKeyState.isKeyUp(DIK_RIGHT))
		{
			ETF++;
			ETF = ETF;
			m_edgeTessellationFactorCB->Update(m_context, ETF);
		}
		else if (prevKeyState.isKeyDown(DIK_LEFT) && currentKeyState.isKeyUp(DIK_LEFT))
		{
			if (ETF > 1) ETF--;
			m_edgeTessellationFactorCB->Update(m_context, ETF);
		}
		else if (prevKeyState.isKeyDown(DIK_UP) && currentKeyState.isKeyUp(DIK_UP))
		{
			ITF++;
			m_interiorTessellationFactorCB->Update(m_context, ITF);
		}
		else if (prevKeyState.isKeyDown(DIK_DOWN) && currentKeyState.isKeyUp(DIK_DOWN))
		{
			if (ITF > 1) ITF--;
			m_interiorTessellationFactorCB->Update(m_context, ITF);
		}
		else if (prevKeyState.isKeyDown(DIK_1) && currentKeyState.isKeyUp(DIK_1))
		{
			wireframeEnabled = !wireframeEnabled;
		}
		else if (prevKeyState.isKeyDown(DIK_2) && currentKeyState.isKeyUp(DIK_2))
		{
			bezierControlNetEnabled = !bezierControlNetEnabled;
		}
		else if (prevKeyState.isKeyDown(DIK_3) && currentKeyState.isKeyUp(DIK_3))
		{
			firstPatchEnabled = !firstPatchEnabled;
		}
		else if (prevKeyState.isKeyDown(DIK_F1) && currentKeyState.isKeyUp(DIK_F1))
		{
			if (part == parts::IVV)
			{
				m_camera.Zoom(-zoomValue);
				UpdateCamera();
			}
			part = parts::I;
		}
		else if (prevKeyState.isKeyDown(DIK_F2) && currentKeyState.isKeyUp(DIK_F2))
		{
			if (part == parts::IVV)
			{
				m_camera.Zoom(-zoomValue);
				UpdateCamera();
			}
			part = parts::II;
		}
		else if (prevKeyState.isKeyDown(DIK_F3) && currentKeyState.isKeyUp(DIK_F3))
		{
			if (part == parts::IVV)
			{
				m_camera.Zoom(-zoomValue);
				UpdateCamera();
			}
			part = parts::III;
		}
		else if (prevKeyState.isKeyDown(DIK_F4) && currentKeyState.isKeyUp(DIK_F4))
		{
			if (part != parts::IVV)
			{
				m_camera.Zoom(zoomValue);
				UpdateCamera();
			}
			part = parts::IVV;
		}
		prevKeyState = currentKeyState;
	}
}

void Tessellation::DrawBezierControlNet()
{
	m_worldCB->Update(m_context, XMMatrixIdentity());
	unsigned int offset = 0;
	m_surfaceColorCB->Update(m_context, XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f));
	m_colorEffect->Begin(m_context);

	m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	ID3D11Buffer* b2 = m_bezierPatchVertexBuffers[!firstPatchEnabled].get();
	m_context->IASetVertexBuffers(0, 1, &b2, &m_vertexStride, &offset);
	m_context->IASetIndexBuffer(m_bezierControlNetIndexBuffer.get(), DXGI_FORMAT_R16_UINT, 0);
	m_context->DrawIndexed(m_bezierPatchIndicesCount, 0, 0);
	m_colorEffect->End();
}

void Tessellation::DrawPartI()
{
	m_context->RSSetState(m_rsWireframe.get());
	m_worldCB->Update(m_context, XMMatrixIdentity());
	unsigned int offset = 0;
	m_surfaceColorCB->Update(m_context, XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f));
	m_partIEffect->Begin(m_context);
	ID3D11Buffer* b = m_quadVertexBuffer.get();

	m_context->IASetVertexBuffers(0, 1, &b, &m_vertexStride, &offset);
	m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST);
	m_context->Draw(m_quadVertexCount, 0);
	m_partIEffect->End();
}

void Tessellation::DrawPartII()
{
	m_context->RSSetState(m_rsWireframe.get());
	m_worldCB->Update(m_context, XMMatrixIdentity());
	unsigned int offset = 0;
	m_surfaceColorCB->Update(m_context, XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f));
	m_partIIEffect->Begin(m_context);
	ID3D11Buffer* b = m_bezierPatchVertexBuffers[!firstPatchEnabled].get();

	m_context->IASetVertexBuffers(0, 1, &b, &m_vertexStride, &offset);
	m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_16_CONTROL_POINT_PATCHLIST);
	m_context->Draw(m_bezierPatchVertexCount, 0);
	m_partIIEffect->End();

	if (bezierControlNetEnabled)
		DrawBezierControlNet();
}

void Tessellation::DrawPartIII()
{
	m_worldCB->Update(m_context, XMMatrixIdentity());
	unsigned int offset = 0;
	m_surfaceColorCB->Update(m_context, XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f));
	m_partIIIEffect->Begin(m_context);
	if (wireframeEnabled)
		m_context->RSSetState(m_rsWireframe.get());
	else
		m_context->RSSetState(m_rsSolid.get());

	ID3D11Buffer* b = m_bezierPatchVertexBuffers[!firstPatchEnabled].get();
	m_context->IASetVertexBuffers(0, 1, &b, &m_vertexStride, &offset);
	m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_16_CONTROL_POINT_PATCHLIST);
	m_context->Draw(m_bezierPatchVertexCount, 0);
	m_partIIIEffect->End();

	if (bezierControlNetEnabled)
		DrawBezierControlNet();
}

void Tessellation::DrawPartIVV()
{
	m_worldCB->Update(m_context, XMMatrixIdentity());
	unsigned int offset = 0;
	m_partIVVEffect->Begin(m_context);
	if (wireframeEnabled)
		m_context->RSSetState(m_rsWireframe.get());
	else
		m_context->RSSetState(m_rsSolid.get());

	for (int i = 0; i < 16; i++)
	{
		m_patchIndexCB->Update(m_context, i);
		ID3D11Buffer* b = m_multipleBezierPatchVertexBuffers[i].get();
		m_context->IASetVertexBuffers(0, 1, &b, &m_vertexStride, &offset);
		m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_16_CONTROL_POINT_PATCHLIST);
		m_context->Draw(m_bezierPatchVertexCount, 0);
	}
	m_partIVVEffect->End();
}

void Tessellation::DrawScene()
{

	switch (part)
	{
	case parts::I:
		DrawPartI();
		break;
	case parts::II:
		DrawPartII();
		break;
	case parts::III:
		DrawPartIII();
		break;
	case parts::IVV:
		DrawPartIVV();
		break;
	default:
		break;
	}
}

void Tessellation::Render()
{
	if (m_context == nullptr)
		return;
	float clearColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	m_context->ClearRenderTargetView(m_backBuffer.get(), clearColor);
	m_context->ClearDepthStencilView(m_depthStencilView.get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	DrawScene();

	m_swapChain->Present(0, 0);
}