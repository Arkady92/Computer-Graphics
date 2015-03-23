#include "gk2_room.h"
#include "gk2_window.h"
#include "gk2_textureGenerator.h"

using namespace std;
using namespace gk2;

const float Room::TABLE_H = 1.0f;
const float Room::TABLE_TOP_H = 0.1f;
const float Room::TABLE_R = 1.5f;
const XMFLOAT3 Room::TABLE_POS = XMFLOAT3(0.5f, -1.96f, 0.5f);
const XMFLOAT4 Room::LIGHT_POS[2] = { XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT4(-1.0f, -1.0f, -1.0f, 1.0f) };

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
	m_lightPosCB.reset(new ConstantBuffer<XMFLOAT4, 2>(m_device));
	m_textureCB.reset(new CBMatrix(m_device));
	m_posterTexCB.reset(new CBMatrix(m_device));
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

void Room::InitializeTextures()
{
	m_wallTexture = m_device.CreateShaderResourceView(L"resources/textures/brick_wall.jpg");
	m_posterTexture = m_device.CreateShaderResourceView(L"resources/textures/lautrec_divan.jpg");
	D3D11_SAMPLER_DESC sd = m_device.DefaultSamplerDesc();
	sd.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sd.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	m_samplerWrap = m_device.CreateSamplerState(sd);
	sd.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	sd.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	sd.BorderColor[0] = 0;
	sd.BorderColor[1] = 0;
	sd.BorderColor[2] = 0;
	sd.BorderColor[3] = 0;
	sd.MipLODBias = 2;
	m_samplerBorder = m_device.CreateSamplerState(sd);
	m_perlinTexture = m_device.CreateShaderResourceView(L"resources/textures/perlin.jpg");

	D3D11_TEXTURE2D_DESC texDesc = m_device.DefaultTexture2DDesc();
	texDesc.Width = 64;
	texDesc.Height = 512;
	texDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
	texDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
	shared_ptr<ID3D11Texture2D> woodTexture = m_device.CreateTexture2D(texDesc);
	shared_ptr<BYTE> data(new BYTE[64 * 512 * 4], Utils::DeleteArray<BYTE>);
	BYTE *d = data.get();
	TextureGenerator txGen(6, 0.35f);
	for (int i = 0; i < 512; ++i)
	{
		float x = i / 512.0f;
		for (int j = 0; j < 64; ++j)
		{
			float y = j / 64.0f;
			float c = txGen.Wood(x, y);
			BYTE ic = static_cast<BYTE>(c * 239);
			*(d++) = ic;
			ic = static_cast<BYTE>(c * 200);
			*(d++) = ic;
			ic = static_cast<BYTE>(c * 139);
			*(d++) = ic;
			*(d++) = 255;
		}
	}
	m_context->UpdateSubresource(woodTexture.get(), 0, 0, data.get(), 64*4, 64*512*4);
	m_woodTexture = m_device.CreateShaderResourceView(woodTexture);
	m_context->GenerateMips(m_woodTexture.get());
}

void Room::CreateScene()
{
	m_walls[0] = m_meshLoader.GetQuad(4.0f);
	for (int i = 1; i < 6; ++i)
		m_walls[i] = m_walls[0];
	XMMATRIX wall = XMMatrixTranslation(0.0f, 0.0f, 2.0f);
	float a = 0;
	for (int i = 0; i < 4; ++i, a += XM_PIDIV2)
		m_walls[i].setWorldMatrix(wall * XMMatrixRotationY(a));
	m_walls[4].setWorldMatrix(wall * XMMatrixRotationX(XM_PIDIV2));
	m_walls[5].setWorldMatrix(wall * XMMatrixRotationX(-XM_PIDIV2));
	m_teapot = m_meshLoader.LoadMesh(L"resources/meshes/teapot.mesh");
	XMMATRIX teapotMtx = XMMatrixTranslation(0.0f, -2.3f, 0.f) * XMMatrixScaling(0.1f, 0.1f, 0.1f) *
						 XMMatrixRotationY(-XM_PIDIV2) * XMMatrixTranslation(-1.3f, -0.74f, -0.6f);
	m_teapot.setWorldMatrix(teapotMtx);
	m_debugSphere = m_meshLoader.GetSphere(8, 16, 0.3f);
	m_debugSphere.setWorldMatrix(/*XMMatrixScaling(0.1f, 0.1f, 0.1f) */
						 XMMatrixRotationY(-XM_PIDIV2) * XMMatrixTranslation(-1.3f, -0.74f, -0.6f));
	m_box = m_meshLoader.GetBox();
	m_box.setWorldMatrix(XMMatrixTranslation(-1.4f, -1.46f, -0.6f));
	m_lamp = m_meshLoader.LoadMesh(L"resources/meshes/lamp.mesh");
	UpdateLamp(0.0f);
	m_chairSeat = m_meshLoader.LoadMesh(L"resources/meshes/chair_seat.mesh");
	m_chairBack = m_meshLoader.LoadMesh(L"resources/meshes/chair_back.mesh");
	XMMATRIX chair = XMMatrixRotationY(XM_PI + XM_PI/9 /*20 deg*/) * XMMatrixTranslation(-0.1f, -1.06f, -1.3f);
	m_chairSeat.setWorldMatrix(chair);
	m_chairBack.setWorldMatrix(chair);
	m_monitor = m_meshLoader.LoadMesh(L"resources/meshes/monitor.mesh");
	m_screen = m_meshLoader.LoadMesh(L"resources/meshes/screen.mesh");
	XMMATRIX monitor = XMMatrixRotationY(XM_PIDIV4) *
					   XMMatrixTranslation(TABLE_POS.x, TABLE_POS.y + TABLE_H + 0.42f, TABLE_POS.z);
	m_monitor.setWorldMatrix(monitor);
	m_screen.setWorldMatrix(monitor);
	m_tableLegs[0] = m_meshLoader.GetCylinder(4, 9, 0.1f, TABLE_H - TABLE_TOP_H);
	for (int i = 1; i < 4; ++i)
		m_tableLegs[i] = m_tableLegs[0];
	a = XM_PIDIV4;
	for (int i = 0; i < 4; ++i, a += XM_PIDIV2)
		m_tableLegs[i].setWorldMatrix(XMMatrixTranslation(0.0f, 0.0f, TABLE_R - 0.35f) * XMMatrixRotationY(a) *
			XMMatrixTranslation(TABLE_POS.x, TABLE_POS.y + (TABLE_H - TABLE_TOP_H) / 2, TABLE_POS.z));
	m_tableSide = m_meshLoader.GetCylinder(1, 16, TABLE_R, TABLE_TOP_H);
	m_tableSide.setWorldMatrix(XMMatrixTranslation(TABLE_POS.x, TABLE_POS.y + TABLE_H - TABLE_TOP_H / 2, TABLE_POS.z));
	m_tableTop = m_meshLoader.GetDisc(16, TABLE_R);
	m_tableTop.setWorldMatrix(XMMatrixTranslation(TABLE_POS.x, TABLE_POS.y + TABLE_H, TABLE_POS.z));
	m_lightPosCB->Update(m_context, LIGHT_POS);
	m_textureCB->Update(m_context, XMMatrixScaling(0.25f, 0.25f, 1.0f) * XMMatrixTranslation(0.5f, 0.5f, 0.0f));

	XMMATRIX posterMatrix = XMMatrixTranslation(-0.5, -0.5, 0) * XMMatrixScaling(1, -4.0 / 3.0, 2) * XMMatrixRotationZ(XM_PI / 18)
		* XMMatrixTranslation(-0.8, 0, 0);
	posterMatrix = XMMatrixInverse(&XMVECTOR(), posterMatrix);

	m_posterTexCB->Update(m_context, posterMatrix);
}

bool Room::LoadContent()
{
	m_meshLoader.setDevice(m_device);
	InitializeConstantBuffers();
	InitializeCamera();
	InitializeTextures();
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
	m_textureEffect->SetTexture(m_wallTexture);

	m_multiTextureEffect.reset(new MultiTextureEffect(m_device, m_layout));
	m_multiTextureEffect->SetProjMtxBuffer(m_projCB);
	m_multiTextureEffect->SetViewMtxBuffer(m_viewCB);
	m_multiTextureEffect->SetWorldMtxBuffer(m_worldCB);
	m_multiTextureEffect->SetTextureMtxBuffer(m_textureCB);
	m_multiTextureEffect->SetTextureMtxBuffer2(m_posterTexCB);
	m_multiTextureEffect->SetSamplerState(m_samplerBorder);
	m_multiTextureEffect->SetTexture(m_wallTexture);
	m_multiTextureEffect->SetTexture2(m_posterTexture);

	m_colorTexEffect.reset(new ColorTexEffect(m_device, m_layout));
	m_colorTexEffect->SetProjMtxBuffer(m_projCB);
	m_colorTexEffect->SetViewMtxBuffer(m_viewCB);
	m_colorTexEffect->SetWorldMtxBuffer(m_worldCB);
	m_colorTexEffect->SetTextureMtxBuffer(m_textureCB);
	m_colorTexEffect->SetSamplerState(m_samplerWrap);
	m_colorTexEffect->SetTexture(m_perlinTexture);
	m_colorTexEffect->SetSurfaceColorBuffer(m_surfaceColorCB);

	m_environmentMapper.reset(new EnvironmentMapper(m_device, m_layout, m_context, 0.4f, 8.0f,
													XMFLOAT3(-1.3f, -0.74f, -0.6f)));
	m_environmentMapper->SetProjMtxBuffer(m_projCB);
	m_environmentMapper->SetViewMtxBuffer(m_viewCB);
	m_environmentMapper->SetWorldMtxBuffer(m_worldCB);
	m_environmentMapper->SetSamplerState(m_samplerWrap);
	m_environmentMapper->SetCameraPosBuffer(m_cameraPosCB);
	m_environmentMapper->SetSurfaceColorBuffer(m_surfaceColorCB);

	D3D11_RASTERIZER_DESC desc = m_device.DefaultRasterizerDesc();
	desc.CullMode = D3D11_CULL_NONE;
	m_rsCullNone = m_device.CreateRasterizerState(desc);
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

void Room::UpdateLamp(float dt)
{
	static float time = 0;
	time += dt;
	float swing = 0.3f * XMScalarSin(XM_2PI*time/8);
	float rot = XM_2PI*time/20;
	XMMATRIX lamp = XMMatrixTranslation(0.0f, -0.4f, 0.0f) * XMMatrixRotationX(swing) * XMMatrixRotationY(rot) *
					XMMatrixTranslation(0.0f, 2.0f, 0.0f);
	m_lamp.setWorldMatrix(lamp);
}

void Room::Update(float dt)
{
	UpdateLamp(dt);
	static MouseState prevState;
	MouseState currentState;
	if (!m_mouse->GetState(currentState))
		return;
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

void Room::DrawWalls()
{
	//Draw floor
	m_textureCB->Update(m_context, XMMatrixScaling(0.25f, 4.0f, 1.0f) * XMMatrixTranslation(0.5f, 0.5f, 0.0f));
	m_textureEffect->SetTexture(m_woodTexture);
	m_textureEffect->Begin(m_context);
	m_worldCB->Update(m_context, m_walls[4].getWorldMatrix());
	m_walls[4].Render(m_context);
	m_textureEffect->End();

	//Draw ceiling
	m_surfaceColorCB->Update(m_context, XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f));
	m_textureCB->Update(m_context, XMMatrixScaling(0.25f, 0.25f, 0.25f) * XMMatrixTranslation(0.5f, 0.5f, 0.0f));
	m_colorTexEffect->Begin(m_context);
	m_worldCB->Update(m_context, m_walls[5].getWorldMatrix());
	m_walls[5].Render(m_context);
	m_colorTexEffect->End();
	m_surfaceColorCB->Update(m_context, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));

	//draw back wall
	//TODO: Replace with multi texture effect
	m_multiTextureEffect->Begin(m_context);
	m_worldCB->Update(m_context, m_walls[0].getWorldMatrix());
	m_walls[0].Render(m_context);
	m_multiTextureEffect->End();

	//draw remaining walls
	m_textureEffect->SetTexture(m_wallTexture);
	m_textureEffect->Begin(m_context);
	for (int i = 1; i < 4; ++i)
	{
		m_worldCB->Update(m_context, m_walls[i].getWorldMatrix());
		m_walls[i].Render(m_context);
	}
	m_textureEffect->End();

}

void Room::DrawTeapot()
{
	//TODO: Replace with environment map effect
	m_phongEffect->Begin(m_context);
	m_surfaceColorCB->Update(m_context, XMFLOAT4(0.8f, 0.7f, 0.65f, 1.0f));

	m_worldCB->Update(m_context, m_teapot.getWorldMatrix());
	m_teapot.Render(m_context);
	/*m_worldCB->Update(m_context, m_debugSphere.getWorldMatrix());
	m_debugSphere.Render(m_context);*/

	m_surfaceColorCB->Update(m_context, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	m_phongEffect->End();
}

void Room::DrawTable()
{
	m_context->RSSetState(m_rsCullNone.get());
	m_surfaceColorCB->Update(m_context, XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f));
	//Draw legs
	for (int i = 0; i < 4; ++i)
	{
		m_worldCB->Update(m_context, m_tableLegs[i].getWorldMatrix());
		m_tableLegs[i].Render(m_context);
	}
	//Draw table surface
	m_worldCB->Update(m_context, m_tableTop.getWorldMatrix());
	m_tableTop.Render(m_context);
	//Draw table top side
	m_worldCB->Update(m_context, m_tableSide.getWorldMatrix());
	m_tableSide.Render(m_context);
	m_surfaceColorCB->Update(m_context, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	m_context->RSSetState(nullptr);
}

void Room::DrawScene()
{

	DrawWalls();
	DrawTeapot();
	m_phongEffect->Begin(m_context);
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
	DrawTable();
	m_phongEffect->End();
}

void Room::Render()
{
	if (m_context == nullptr)
		return;

	//TODO: Render scene to each environment cube map face

	ResetRenderTarget();
	m_projCB->Update(m_context, m_projMtx);
	UpdateCamera();
	//Clear buffers
	float clearColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	m_context->ClearRenderTargetView(m_backBuffer.get(), clearColor);
	m_context->ClearDepthStencilView(m_depthStencilView.get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	DrawScene();
	m_swapChain->Present(0, 0);
}
