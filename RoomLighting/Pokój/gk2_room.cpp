#include "gk2_room.h"
#include "gk2_window.h"

using namespace std;
using namespace gk2;

const unsigned int Room::BS_MASK = 0xffffffff;
const float Room::TABLE_H = 1.0f;
const float Room::TABLE_TOP_H = 0.1f;
const float Room::TABLE_R = 1.0f;
const XMFLOAT4 Room::TABLE_POS = XMFLOAT4(0.8f, -1.46f, 0.7f, 1.0f);

Room::Room(HINSTANCE hInstance)
	: ApplicationBase(hInstance), m_camera(0.01f, 100.0f)
{

}

Room::~Room()
{

}

void* Room::operator new(size_t size)
{
	return Utils::New16Aligned(size);
}

void Room::operator delete(void* ptr)
{
	Utils::Delete16Aligned(ptr);
}

void Room::InitializeConstantBuffers()
{
	m_projCB.reset(new CBMatrix(m_device));
	m_viewCB.reset(new CBMatrix(m_device));
	m_worldCB.reset(new CBMatrix(m_device));
	m_lightPosCB.reset(new ConstantBuffer<XMFLOAT4>(m_device));
	m_surfaceColorCB.reset(new ConstantBuffer<XMFLOAT4>(m_device));
	m_cameraPosCB.reset(new ConstantBuffer<XMFLOAT4>(m_device));
}

void Room::InitializeCamera()
{
	SIZE s = getMainWindow()->getClientSize();
	float ar = static_cast<float>(s.cx) / s.cy;
	m_projMtx = XMMatrixPerspectiveFovLH(XM_PIDIV4, ar, 0.01f, 100.0f);
	m_projCB->Update(m_context, m_projMtx);
	m_camera.Zoom(5);
	UpdateCamera();
}

void Room::CreateScene()
{
	m_walls[0] = m_meshLoader.GetQuad(5.0f);
	for (int i = 1; i < 6; ++i)
		m_walls[i] = m_walls[0];
	XMMATRIX wall = XMMatrixTranslation(0.0f, 0.0f, 2.5f);
	float a = 0;
	for (int i = 0; i < 4; ++i, a += XM_PIDIV2)
		m_walls[i].setWorldMatrix(wall * XMMatrixRotationY(a));
	m_walls[4].setWorldMatrix(wall * XMMatrixRotationX(XM_PIDIV2));
	m_walls[5].setWorldMatrix(wall * XMMatrixRotationX(-XM_PIDIV2));
	m_teapot = m_meshLoader.LoadMesh(L"resources/meshes/teapot.mesh");
	XMMATRIX teapotMtx = XMMatrixTranslation(0.0f, -2.3f, 0.f) * XMMatrixScaling(0.1f, 0.1f, 0.1f) *
						 XMMatrixRotationY(-XM_PIDIV2) * XMMatrixTranslation(-1.0f, -1.24f, 0.0f);
	m_teapot.setWorldMatrix(teapotMtx);
	m_debugSphere = m_meshLoader.GetSphere(8, 16, 0.3f);
	m_debugSphere.setWorldMatrix(/*XMMatrixScaling(0.1f, 0.1f, 0.1f) */
						 XMMatrixRotationY(-XM_PIDIV2) * XMMatrixTranslation(-1.0f, -1.24f, 0.0f));
	m_box = m_meshLoader.GetBox();
	m_box.setWorldMatrix(XMMatrixTranslation(-1.1f, -1.96f, 0.0f));
	m_lamp = m_meshLoader.LoadMesh(L"resources/meshes/lamp.mesh");
	m_chairSeat = m_meshLoader.LoadMesh(L"resources/meshes/chair_seat.mesh");
	m_chairBack = m_meshLoader.LoadMesh(L"resources/meshes/chair_back.mesh");
	XMMATRIX chair = XMMatrixRotationY(XM_PI + XM_PI/9 /*20 deg*/) * XMMatrixTranslation(-0.1f, -1.56f, -1.3f);
	m_chairSeat.setWorldMatrix(chair);
	m_chairBack.setWorldMatrix(chair);
	m_monitor = m_meshLoader.LoadMesh(L"resources/meshes/monitor.mesh");
	m_screen = m_meshLoader.LoadMesh(L"resources/meshes/screen.mesh");
	XMMATRIX monitor = XMMatrixRotationY(XM_PIDIV4) *
					   XMMatrixTranslation(TABLE_POS.x, TABLE_POS.y + 0.42f, TABLE_POS.z);
	m_monitor.setWorldMatrix(monitor);
	m_screen.setWorldMatrix(monitor);
	m_tableLegs[0] = m_meshLoader.GetCylinder(4, 9, 0.1f, TABLE_H - TABLE_TOP_H);
	for (int i = 1; i < 4; ++i)
		m_tableLegs[i] = m_tableLegs[0];
	a = XM_PIDIV4;
	for (int i = 0; i < 4; ++i, a += XM_PIDIV2)
		m_tableLegs[i].setWorldMatrix(XMMatrixTranslation(0.0f, 0.0f, TABLE_R - 0.35f) * XMMatrixRotationY(a) *
			XMMatrixTranslation(TABLE_POS.x, TABLE_POS.y - (TABLE_H + TABLE_TOP_H) / 2, TABLE_POS.z));
	m_tableSide = m_meshLoader.GetCylinder(1, 16, TABLE_R, TABLE_TOP_H);
	m_tableSide.setWorldMatrix(XMMatrixRotationY(XM_PIDIV4/4) *
							   XMMatrixTranslation(TABLE_POS.x, TABLE_POS.y - TABLE_TOP_H / 2, TABLE_POS.z));
	m_tableTop = m_meshLoader.GetDisc(16, TABLE_R);
	m_tableTop.setWorldMatrix(XMMatrixRotationY(XM_PIDIV4/4) *
							  XMMatrixTranslation(TABLE_POS.x, TABLE_POS.y, TABLE_POS.z));
	m_surfaceColorCB->Update(m_context, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
}

void Room::InitializeRenderStates()
{
	D3D11_RASTERIZER_DESC rsDesc = m_device.DefaultRasterizerDesc();
	rsDesc.CullMode = D3D11_CULL_NONE;
	m_rsCullNone = m_device.CreateRasterizerState(rsDesc);

	D3D11_BLEND_DESC bsDesc = m_device.DefaultBlendDesc();
	bsDesc.RenderTarget[0].BlendEnable = true;
	bsDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	bsDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	bsDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	m_bsAlpha = m_device.CreateBlendState(bsDesc);

	D3D11_DEPTH_STENCIL_DESC dssDesc = m_device.DefaultDepthStencilDesc();
	dssDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	m_dssNoWrite = m_device.CreateDepthStencilState(dssDesc);
}

bool Room::LoadContent()
{
	m_meshLoader.setDevice(m_device);
	InitializeConstantBuffers();
	InitializeCamera();
	InitializeRenderStates();
	m_meshLoader.setDevice(m_device);
	CreateScene();
	m_phongEffect.reset(new PhongEffect(m_device, m_layout));
	m_phongEffect->SetProjMtxBuffer(m_projCB);
	m_phongEffect->SetViewMtxBuffer(m_viewCB);
	m_phongEffect->SetWorldMtxBuffer(m_worldCB);
	m_phongEffect->SetLightPosBuffer(m_lightPosCB);
	m_phongEffect->SetSurfaceColorBuffer(m_surfaceColorCB);

	m_lightShadowEffect.reset(new LightShadowEffect(m_device, m_layout));
	m_lightShadowEffect->SetProjMtxBuffer(m_projCB);
	m_lightShadowEffect->SetViewMtxBuffer(m_viewCB);
	m_lightShadowEffect->SetWorldMtxBuffer(m_worldCB);
	m_lightShadowEffect->SetLightPosBuffer(m_lightPosCB);
	m_lightShadowEffect->SetSurfaceColorBuffer(m_surfaceColorCB);
	m_lamp.setWorldMatrix(m_lightShadowEffect->UpdateLight(0.0f, m_context));

	m_particles.reset(new ParticleSystem(m_device,  XMFLOAT3(-1.0f, -1.1f, 0.46f)));
	m_particles->SetViewMtxBuffer(m_viewCB);
	m_particles->SetProjMtxBuffer(m_projCB);
	return true;
}

void Room::UnloadContent()
{

}

void Room::UpdateCamera()
{
	XMMATRIX view;
	m_camera.GetViewMatrix(view);
	m_viewCB->Update(m_context, view);
	m_cameraPosCB->Update(m_context, m_camera.GetPosition());
}


void Room::Update(float dt)
{
	m_lamp.setWorldMatrix(m_lightShadowEffect->UpdateLight(dt, m_context));
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
	m_particles->Update(m_context, dt, m_camera.GetPosition());
}

void Room::DrawScene()
{
	//Draw walls
	for (int i = 0; i < 6; ++i)
	{
		m_worldCB->Update(m_context, m_walls[i].getWorldMatrix());
		m_walls[i].Render(m_context);
	}
	//Draw teapot
	m_worldCB->Update(m_context, m_teapot.getWorldMatrix());
	m_teapot.Render(m_context);
	m_surfaceColorCB->Update(m_context, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	//Draw shelf
	m_worldCB->Update(m_context, m_box.getWorldMatrix());
	m_box.Render(m_context);
	//Draw lamp
	m_worldCB->Update(m_context, m_lamp.getWorldMatrix());
	m_lamp.Render(m_context);
	//Draw chair seat
	m_worldCB->Update(m_context, m_chairSeat.getWorldMatrix());
	m_chairSeat.Render(m_context);
	//Draw chairframe
	m_worldCB->Update(m_context, m_chairBack.getWorldMatrix());
	m_chairBack.Render(m_context);
	//Draw monitor
	m_worldCB->Update(m_context, m_monitor.getWorldMatrix());
	m_monitor.Render(m_context);
	//Draw screen
	m_worldCB->Update(m_context, m_screen.getWorldMatrix());
	m_screen.Render(m_context);
	//Draw table
	m_context->RSSetState(m_rsCullNone.get());
	//Table top
	m_worldCB->Update(m_context, m_tableTop.getWorldMatrix());
	m_tableTop.Render(m_context);
	//Table side
	m_worldCB->Update(m_context, m_tableSide.getWorldMatrix());
	m_tableSide.Render(m_context);
	//Table legs
	for (int i = 0; i < 4; ++i)
	{
		m_worldCB->Update(m_context, m_tableLegs[i].getWorldMatrix());
		m_tableLegs[i].Render(m_context);
	}
	m_context->RSSetState(nullptr);
}

void Room::Render()
{
	if (m_context == nullptr)
		return;

	m_lightShadowEffect->SetupShadow(m_context);
	m_phongEffect->Begin(m_context);
	DrawScene();
	m_phongEffect->End();
	m_particles->Render(m_context);
	m_lightShadowEffect->EndShadow();

	ResetRenderTarget();
	m_projCB->Update(m_context, m_projMtx);
	UpdateCamera();
	//Clear buffers
	float clearColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	m_context->ClearRenderTargetView(m_backBuffer.get(), clearColor);
	m_context->ClearDepthStencilView(m_depthStencilView.get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	m_lightShadowEffect->Begin(m_context);
	DrawScene();
	m_lightShadowEffect->End();
	
	m_context->OMSetBlendState(m_bsAlpha.get(), nullptr, BS_MASK);
	m_context->OMSetDepthStencilState(m_dssNoWrite.get(), 0);
	m_particles->Render(m_context);
	m_context->OMSetDepthStencilState(nullptr, 0);
	m_context->OMSetBlendState(nullptr, nullptr, BS_MASK);
	m_swapChain->Present(0, 0);
}
