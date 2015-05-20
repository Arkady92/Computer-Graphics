#include "gk2_room.h"
#include "gk2_window.h"

using namespace std;
using namespace gk2;

const unsigned int Room::BS_MASK = 0xffffffff;
const XMFLOAT4 Room::LIGHT_POS = XMFLOAT4(-5.0f, 5.0f, -5.0f, 1.0f);

Room::Room(HINSTANCE hInstance)
	: ApplicationBase(hInstance), m_camera(0.01f, 100.0f), angle(0.0f)
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
	m_textureCB.reset(new CBMatrix(m_device));
	m_lightPosCB.reset(new ConstantBuffer<XMFLOAT4>(m_device));
	m_surfaceColorCB.reset(new ConstantBuffer<XMFLOAT4>(m_device));
	m_cameraPosCB.reset(new ConstantBuffer<XMFLOAT4>(m_device));
}

void Room::InitializeTextures()
{
	m_forestTexture = m_device.CreateShaderResourceView(L"resources/textures/forest.jpg");
	m_skyTexture = m_device.CreateShaderResourceView(L"resources/textures/sky.png");
	m_bottomTexture = m_device.CreateShaderResourceView(L"resources/textures/bottom.jpg");
	D3D11_SAMPLER_DESC sd = m_device.DefaultSamplerDesc();
	sd.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sd.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	m_samplerWrap = m_device.CreateSamplerState(sd);
}

void Room::InitializeCamera()
{
	SIZE s = getMainWindow()->getClientSize();
	float ar = static_cast<float>(s.cx) / s.cy;
	m_projMtx = XMMatrixPerspectiveFovLH(XM_PIDIV4, ar, 0.01f, 100.0f);
	m_projCB->Update(m_context, m_projMtx);
	m_camera.MoveCamera(0.0f, 10.0f, 10.0f);
	m_camera.Rotate(0.65f, 0.785f);
	UpdateCamera();
}

void Room::CreateScene()
{
	// walls
	m_walls[0] = m_meshLoader.GetQuad(10.0f);
	for (int i = 1; i < 6; ++i)
		m_walls[i] = m_walls[0];
	XMMATRIX wall = XMMatrixTranslation(0.0f, 0.0f, 5.0f);
	float a = 0;
	for (int i = 0; i < 4; ++i, a += XM_PIDIV2)
		m_walls[i].setWorldMatrix(wall * XMMatrixRotationY(a));
	m_walls[4].setWorldMatrix(wall * XMMatrixRotationX(XM_PIDIV2));
	m_walls[5].setWorldMatrix(wall * XMMatrixRotationX(-XM_PIDIV2));

	m_lightPosCB->Update(m_context, LIGHT_POS);
	m_textureCB->Update(m_context, XMMatrixScaling(0.25f, 0.25f, 1.0f) * XMMatrixTranslation(0.5f, 0.5f, 0.0f));
}

void Room::UpdateDuck(float dt)
{
	
}

void Room::InitializeRenderStates()
{
	D3D11_RASTERIZER_DESC rsDesc = m_device.DefaultRasterizerDesc();
	rsDesc.CullMode = D3D11_CULL_NONE;
	m_rsCullNone = m_device.CreateRasterizerState(rsDesc);

	rsDesc.FrontCounterClockwise = true;
	m_rsCounterClockwise = m_device.CreateRasterizerState(rsDesc);

	D3D11_BLEND_DESC bsDesc = m_device.DefaultBlendDesc();
	bsDesc.RenderTarget[0].BlendEnable = true;
	bsDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	bsDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	bsDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	m_bsAlpha = m_device.CreateBlendState(bsDesc);

	D3D11_DEPTH_STENCIL_DESC dssDesc = m_device.DefaultDepthStencilDesc();
	dssDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	m_dssNoWrite = m_device.CreateDepthStencilState(dssDesc);
	dssDesc.StencilEnable = true;
	dssDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	dssDesc.BackFace.StencilFunc = D3D11_COMPARISON_NEVER;
	dssDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	dssDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
	m_dssWrite = m_device.CreateDepthStencilState(dssDesc);

	dssDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dssDesc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
	dssDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	m_dssTest = m_device.CreateDepthStencilState(dssDesc);
}

bool Room::LoadContent()
{
	m_meshLoader.setDevice(m_device);
	InitializeConstantBuffers();
	InitializeCamera();
	InitializeTextures();
	InitializeRenderStates();
	m_meshLoader.setDevice(m_device);
	CreateScene();

	m_phongEffect.reset(new PhongEffect(m_device, m_layout));
	m_phongEffect->SetProjMtxBuffer(m_projCB);
	m_phongEffect->SetViewMtxBuffer(m_viewCB);
	m_phongEffect->SetWorldMtxBuffer(m_worldCB);
	m_phongEffect->SetLightPosBuffer(m_lightPosCB);
	m_phongEffect->SetSurfaceColorBuffer(m_surfaceColorCB);

	m_textureEffect.reset(new TextureEffect(m_device, m_layout));
	m_textureEffect->SetProjMtxBuffer(m_projCB);
	m_textureEffect->SetViewMtxBuffer(m_viewCB);
	m_textureEffect->SetWorldMtxBuffer(m_worldCB);
	m_textureEffect->SetTextureMtxBuffer(m_textureCB);
	m_textureEffect->SetSamplerState(m_samplerWrap);

	return true;
}

void Room::UnloadContent()
{

}

void Room::UpdateCamera()
{
	XMMATRIX view;

	CheckKeys(m_camera);

	m_camera.GetViewMatrix(view);
	m_viewCB->Update(m_context, view);
	m_cameraPosCB->Update(m_context, m_camera.GetPosition());

}

void Room::CheckKeys(Camera& m_camera){

	KeyboardState currentKeyboardState;
	if (m_keyboard->GetState(currentKeyboardState)){

		if (currentKeyboardState.isKeyDown(DIK_Q) || currentKeyboardState.isKeyDown(DIK_RSHIFT)){
			m_camera.MoveCamera(0, 0, 1 / 300.f);
		}

		if (currentKeyboardState.isKeyDown(DIK_E) || currentKeyboardState.isKeyDown(DIK_RCONTROL)){
			m_camera.MoveCamera(0, 0, -1 / 300.f);
		}

		if (currentKeyboardState.isKeyDown(DIK_W) || currentKeyboardState.isKeyDown(DIK_UP)){
			m_camera.MoveCamera(1 / 300.f, 0, 0);
		}

		if (currentKeyboardState.isKeyDown(DIK_S) || currentKeyboardState.isKeyDown(DIK_DOWN)){
			m_camera.MoveCamera(-1 / 300.f, 0, 0);
		}

		if (currentKeyboardState.isKeyDown(DIK_A) || currentKeyboardState.isKeyDown(DIK_LEFT)){
			m_camera.MoveCamera(0, 1 / 300.f, 0);
		}

		if (currentKeyboardState.isKeyDown(DIK_D) || currentKeyboardState.isKeyDown(DIK_RIGHT)){
			m_camera.MoveCamera(0, -1 / 300.f, 0);
		}
		
	}

}

void Room::Update(float dt)
{
	static MouseState prevState;
	MouseState currentState;
	if (m_mouse->GetState(currentState))
	{
		bool change = true;
		if (prevState.isButtonDown(0))
		{
			POINT d = currentState.getMousePositionChange();
			m_camera.Rotate(d.y / 400.f, d.x / 400.f);
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

	CheckKeys(m_camera);

	UpdateDuck(dt);
}

void Room::DrawWalls()
{
	m_textureEffect->SetTexture(m_forestTexture);
	m_textureEffect->Begin(m_context);
	for (size_t i = 0; i < 4; i++)
	{
		m_worldCB->Update(m_context, m_walls[i].getWorldMatrix());
		m_walls[i].Render(m_context);
	}
	m_textureEffect->End();

	m_textureEffect->SetTexture(m_skyTexture);
	m_textureEffect->Begin(m_context);
	m_worldCB->Update(m_context, m_walls[5].getWorldMatrix());
	m_walls[5].Render(m_context);
	m_textureEffect->End();

	m_textureEffect->SetTexture(m_bottomTexture);
	m_textureEffect->Begin(m_context);
	m_worldCB->Update(m_context, m_walls[4].getWorldMatrix());
	m_walls[4].Render(m_context);
	m_textureEffect->End();

	//m_surfaceColorCB->Update(m_context, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	//m_phongEffect->Begin(m_context);
	//m_worldCB->Update(m_context, m_cylinder.getWorldMatrix());
	//m_cylinder.Render(m_context);
	//m_phongEffect->End();

	//m_context->OMSetBlendState(m_bsAlpha.get(), nullptr, BS_MASK);
	//m_context->OMSetDepthStencilState(m_dssNoWrite.get(), 0);
	//m_surfaceColorCB->Update(m_context, XMFLOAT4(1.0f, 0.0f, 0.0f, 0.35f));
	//m_phongEffect->Begin(m_context);
	//m_worldCB->Update(m_context, m_circle.getWorldMatrix());
	//m_circle.Render(m_context);
	//m_phongEffect->End();
	//m_context->OMSetDepthStencilState(nullptr, 0);
	//m_context->OMSetBlendState(nullptr, nullptr, BS_MASK);
}

void Room::DrawScene()
{
	DrawWalls();

	m_context->RSSetState(nullptr);
}

void Room::Render()
{
	if (m_context == nullptr)
		return;

	ResetRenderTarget();
	m_projCB->Update(m_context, m_projMtx);
	UpdateCamera();

	float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	m_context->ClearRenderTargetView(m_backBuffer.get(), clearColor);
	m_context->ClearDepthStencilView(m_depthStencilView.get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	DrawScene();

	m_swapChain->Present(0, 0);
}
