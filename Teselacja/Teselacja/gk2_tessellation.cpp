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
}

void Tessellation::InitializeCamera()
{
	SIZE s = getMainWindow()->getClientSize();
	float ar = static_cast<float>(s.cx) / s.cy;
	m_projMtx = XMMatrixPerspectiveFovLH(XM_PIDIV4, ar, 0.01f, 100.0f);
	m_projCB->Update(m_context, m_projMtx);
	m_camera.Zoom(5);
	m_camera.Rotate(-XM_PIDIV4/2,0);
	UpdateCamera();
}

void Tessellation::InitializeRenderStates()
{
	D3D11_RASTERIZER_DESC rsDesc = m_device.DefaultRasterizerDesc();
	rsDesc.CullMode = D3D11_CULL_NONE;
	rsDesc.FillMode = D3D11_FILL_WIREFRAME;
	m_rsWireframe = m_device.CreateRasterizerState(rsDesc);
	
}

bool Tessellation::LoadContent()
{
	InitializeConstantBuffers();
	InitializeRenderStates();
	InitializeCamera();

	m_triangleEffect.reset(new TriangleEffect(m_device, m_layout));
	m_triangleEffect->SetProjMtxBuffer(m_projCB);
	m_triangleEffect->SetViewMtxBuffer(m_viewCB);
	m_triangleEffect->SetWorldMtxBuffer(m_worldCB);
	m_triangleEffect->SetSurfaceColorBuffer(m_surfaceColorCB);

	VertexPos vertices[3] = 
	{
		{ XMFLOAT3(-2.0f, -1.5f, 0.0f) },
		{ XMFLOAT3(2.0f, -1.5f, 0.0f) },
		{ XMFLOAT3(0.0f, 1.5f, 0.0f) }
	};
	m_vertexCount = 3;
	m_vertexBuffer = m_device.CreateVertexBuffer(vertices, m_vertexCount);
	m_vertexStride = sizeof(VertexPos);
	return true;
}

void Tessellation::UnloadContent()
{

}

void Tessellation::UpdateCamera()
{
	XMMATRIX view;
	m_camera.GetViewMatrix(view);
	m_viewCB->Update(m_context, view);
}

void Tessellation::Update(float dt)
{
	static MouseState prevState;
	MouseState currentState;
	if (m_mouse->GetState(currentState))
	{
		bool change = true;
		if (prevState.isButtonDown(0))
		{
			POINT d = currentState.getMousePositionChange();
			m_camera.Rotate(d.y/300.f, d.x/300.f);
		}
		else if (prevState.isButtonDown(1))
		{
			POINT d = currentState.getMousePositionChange();
			m_camera.Zoom(d.y/10.0f);
		}
		else
			change = false;
		prevState = currentState;
		if (change)
			UpdateCamera();
	}
}

void Tessellation::DrawScene()
{
	m_context->RSSetState(m_rsWireframe.get());
	m_worldCB->Update(m_context, XMMatrixIdentity());
	m_surfaceColorCB->Update(m_context, XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f));

	m_triangleEffect->Begin(m_context);
	unsigned int offset = 0;
	ID3D11Buffer* b = m_vertexBuffer.get();
	m_context->IASetVertexBuffers(0, 1, &b, &m_vertexStride, &offset);
	m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
	m_context->Draw(m_vertexCount, 0);
	m_triangleEffect->End();
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