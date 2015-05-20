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
	m_wallTexture = m_device.CreateShaderResourceView(L"resources/textures/stones.jpg");
	m_sunTexture = m_device.CreateShaderResourceView(L"resources/textures/sun.jpg");
	D3D11_SAMPLER_DESC sd = m_device.DefaultSamplerDesc();
	sd.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sd.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	m_samplerWrap = m_device.CreateSamplerState(sd);
	sd.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	sd.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	m_samplerBorder = m_device.CreateSamplerState(sd);
	m_steelSheetTexture = m_device.CreateShaderResourceView(L"resources/textures/metal.jpg");
}

void Room::InitializeCamera()
{
	SIZE s = getMainWindow()->getClientSize();
	float ar = static_cast<float>(s.cx) / s.cy;
	m_projMtx = XMMatrixPerspectiveFovLH(XM_PIDIV4, ar, 0.01f, 100.0f);
	m_projCB->Update(m_context, m_projMtx);
	m_camera.MoveCamera(25.f, 12.5f, 5.0f);
	m_camera.Rotate(0.25f, 2.45f);
	m_camera.Zoom(50);
	UpdateCamera();
}

void Room::CreateScene()
{
	// walls
	m_walls[0] = m_meshLoader.GetQuad(10.0f, 10.0f);
	for (int i = 1; i < 6; ++i)
		m_walls[i] = m_walls[0];
	XMMATRIX wall = XMMatrixTranslation(0.0f, 0.0f, 5.0f);
	XMMATRIX mWall = XMMatrixTranslation(0.0f, 3.99f, 0.0f);
	float a = 0;
	for (int i = 0; i < 4; ++i, a += XM_PIDIV2)
		m_walls[i].setWorldMatrix(wall * XMMatrixRotationY(a) * mWall);
	m_walls[4].setWorldMatrix(wall * XMMatrixRotationX(XM_PIDIV2)* mWall);
	m_walls[5].setWorldMatrix(wall * XMMatrixRotationX(-XM_PIDIV2)* mWall);

	// cyllinder
	m_cylinder = m_meshLoader.GetCylinder(100, 100, 0.25f, 2.5f);
	m_cylinder.setWorldMatrix(XMMatrixRotationZ(XM_PIDIV2) * XMMatrixTranslation(0.0f, -0.75f, 1.5f));

	// sun
	m_sun = m_meshLoader.GetSphere(100, 100, 0.5);
	m_sun.setWorldMatrix(XMMatrixTranslation(LIGHT_POS.x, LIGHT_POS.y, LIGHT_POS.z));

	// steel sheet
	steelWidth = 2.0f;
	XMMATRIX steelSheetMatrix = XMMatrixRotationY(-XM_PIDIV2) * XMMatrixRotationZ(XM_PI / 6)
		* XMMatrixTranslation(-1.7f, 0.0f, 0.0f);

	m_steelSheet = m_meshLoader.GetQuad(steelWidth, steelWidth);
	m_steelSheet.setWorldMatrix(steelSheetMatrix);

	//mirror
	m_mirror = m_meshLoader.GetQuad(steelWidth);
	m_mirror.setWorldMatrix(steelSheetMatrix);
	m_mirrorMtx = XMMatrixInverse(&XMVECTOR(), steelSheetMatrix) * XMMatrixScaling(1, 1, -1) * steelSheetMatrix;

	// Circle
	m_circle = m_meshLoader.GetRim(100, steelWidth / 2.65);
	m_circle.setWorldMatrix(XMMatrixTranslation(0.0f, 0.01f, 0.0f) * XMMatrixRotationX(-XM_PIDIV2) * steelSheetMatrix);


	// puma
	m_mesh1 = m_meshLoader.LoadMeshForPuma(L"resources/meshes/mesh1.txt", m_shadowVolumes[0], LIGHT_POS);
	m_mesh2 = m_meshLoader.LoadMeshForPuma(L"resources/meshes/mesh2.txt", m_shadowVolumes[1], LIGHT_POS);
	m_mesh3 = m_meshLoader.LoadMeshForPuma(L"resources/meshes/mesh3.txt", m_shadowVolumes[2], LIGHT_POS);
	m_mesh4 = m_meshLoader.LoadMeshForPuma(L"resources/meshes/mesh4.txt", m_shadowVolumes[3], LIGHT_POS);
	m_mesh5 = m_meshLoader.LoadMeshForPuma(L"resources/meshes/mesh5.txt", m_shadowVolumes[4], LIGHT_POS);
	m_mesh6 = m_meshLoader.LoadMeshForPuma(L"resources/meshes/mesh6.txt", m_shadowVolumes[5], LIGHT_POS);


	m_lightPosCB->Update(m_context, LIGHT_POS);
	m_textureCB->Update(m_context, XMMatrixScaling(0.25f, 0.25f, 1.0f) * XMMatrixTranslation(0.5f, 0.5f, 0.0f));
}



void Room::UpdatePuma(float dt)
{
	angle = angle + 3 * dt;

	if (angle > XM_2PI){
		angle -= XM_2PI;
	}

	XMFLOAT3 electrodePosition = XMFLOAT3(0.75f * cosf(angle), 0.75f * sinf(angle), 0.0f);

	XMVECTOR electrodePositionVector = XMLoadFloat3(&electrodePosition);

	XMVECTOR position = XMVector3Transform(electrodePositionVector, m_steelSheet.getWorldMatrix());
	XMStoreFloat3(&electrodePosition, position);

	XMFLOAT4 basicElectrodeNormal = XMFLOAT4(0.0f, 0.0f, -1.0f, 0.0f);
	XMVECTOR transformedElectrodeNormal = XMVector4Transform(XMLoadFloat4(&basicElectrodeNormal), m_steelSheet.getWorldMatrix());
	float a1, a2, a3, a4, a5;

	InversedKinematic(position, transformedElectrodeNormal, a1, a2, a3, a4, a5);
	
	XMMATRIX result;

	XMMATRIX m_mesh2_matrix2 = XMMatrixRotationY(a1);
	m_mesh2.setWorldMatrix(m_mesh2_matrix2);

	XMMATRIX m_mesh3_matrix3 = XMMatrixTranslation(0.0f, -0.27f, 0.0f) * XMMatrixRotationZ(a2) * XMMatrixTranslation(0.0f, 0.27f, 0.0f);
	result = m_mesh3_matrix3 * m_mesh2_matrix2;
	m_mesh3.setWorldMatrix(result);

	XMMATRIX m_mesh4_matrix4 = XMMatrixTranslation(0.91f, -0.27f, 0.0f) * XMMatrixRotationZ(a3) * XMMatrixTranslation(-0.91f, 0.27f, 0.0f);
	result = m_mesh4_matrix4 * m_mesh3_matrix3 * m_mesh2_matrix2;
	m_mesh4.setWorldMatrix(result);

	XMMATRIX m_mesh_matrix5 = XMMatrixTranslation(0.0f, -0.27f, 0.26f) * XMMatrixRotationX(a4) * XMMatrixTranslation(0.0f, 0.27f, -0.26f);
	result = m_mesh_matrix5 * m_mesh4_matrix4 * m_mesh3_matrix3 * m_mesh2_matrix2;
	m_mesh5.setWorldMatrix(result);

	XMMATRIX m_mesh_matrix6 = XMMatrixTranslation(1.72f, -0.27f, 0.0f) * XMMatrixRotationZ(a5) * XMMatrixTranslation(-1.72f, 0.27f, 0.0f);
	result = m_mesh_matrix6 * m_mesh_matrix5 * m_mesh4_matrix4 * m_mesh3_matrix3 * m_mesh2_matrix2;
	m_mesh6.setWorldMatrix(result);

	m_particles->Update(m_context, dt, m_camera.GetPosition(), electrodePosition);

}

void Room::InversedKinematic(XMVECTOR pos, XMVECTOR normal, float &a1, float &a2, float &a3, float &a4, float &a5)
{
	float l1 = .91f, l2 = .81f, l3 = .33f, dy = .27f, dz = .26f;
	XMVECTOR norm1 = XMVector3Normalize(normal);
	XMVECTOR pos1 = pos + norm1 * l3;
	XMFLOAT3 pos1f;
	XMStoreFloat3(&pos1f, pos1);
	float e = sqrtf(pos1f.z*pos1f.z + pos1f.x*pos1f.x - dz*dz);
	a1 = atan2(pos1f.z, -pos1f.x) + atan2(dz, e);
	XMFLOAT3 pos2f = XMFLOAT3(e, pos1f.y - dy, .0f);
	XMVECTOR pos2 = XMLoadFloat3(&pos2f);
	a3 = -acosf(min(1.0f, (pos2f.x*pos2f.x + pos2f.y*pos2f.y - l1*l1 - l2*l2) / (2.0f*l1*l2)));
	float k = l1 + l2 * cosf(a3), l = l2 * sinf(a3);
	a2 = -atan2(pos2f.y, sqrtf(pos2f.x*pos2f.x + pos2f.z*pos2f.z)) - atan2(l, k);
	XMVECTOR normal1 = XMVector3Transform(norm1, XMMatrixRotationY(-a1));
	normal1 = XMVector3Transform(normal1, XMMatrixRotationZ(-(a2 + a3)));
	XMFLOAT3 normal1f;
	XMStoreFloat3(&normal1f, normal1);
	a5 = acosf(normal1f.x);
	a4 = atan2(normal1f.z, normal1f.y);
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

	bsDesc = m_device.DefaultBlendDesc();
	bsDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	m_bsAll = m_device.CreateBlendState(bsDesc);

	bsDesc = m_device.DefaultBlendDesc();
	bsDesc.RenderTarget[0].RenderTargetWriteMask = 0;
	m_bsNone = m_device.CreateBlendState(bsDesc);

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

	rsDesc = m_device.DefaultRasterizerDesc();
	rsDesc.CullMode = D3D11_CULL_BACK;
	m_rsCullBack = m_device.CreateRasterizerState(rsDesc);

	rsDesc = m_device.DefaultRasterizerDesc();
	rsDesc.CullMode = D3D11_CULL_FRONT;
	m_rsCullFront = m_device.CreateRasterizerState(rsDesc);

	rsDesc = m_device.DefaultRasterizerDesc();
	m_rsDefault = m_device.CreateRasterizerState(rsDesc);

	dssDesc = m_device.DefaultDepthStencilDesc();
	dssDesc.StencilEnable = true;
	dssDesc.DepthEnable = true;
	dssDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	dssDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	dssDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_INCR;
	dssDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dssDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	dssDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	dssDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_INCR;
	dssDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dssDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	m_dssIncr = m_device.CreateDepthStencilState(dssDesc);

	dssDesc = m_device.DefaultDepthStencilDesc();
	dssDesc.StencilEnable = true;
	dssDesc.DepthEnable = true;
	dssDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	dssDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	dssDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dssDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	dssDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_DECR;
	dssDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	dssDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dssDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	dssDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_DECR;
	m_dssDecr = m_device.CreateDepthStencilState(dssDesc);

	dssDesc = m_device.DefaultDepthStencilDesc();
	dssDesc.StencilEnable = true;
	dssDesc.DepthEnable = true;
	dssDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dssDesc.DepthFunc = D3D11_COMPARISON_GREATER_EQUAL;
	dssDesc.BackFace.StencilFunc = D3D11_COMPARISON_GREATER;
	dssDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dssDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dssDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	dssDesc.FrontFace.StencilFunc = D3D11_COMPARISON_GREATER;
	dssDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dssDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dssDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	m_dssKeepGreather = m_device.CreateDepthStencilState(dssDesc);

	dssDesc.BackFace.StencilFunc = D3D11_COMPARISON_EQUAL;
	dssDesc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
	m_dssKeepEqual = m_device.CreateDepthStencilState(dssDesc);

	dssDesc = m_device.DefaultDepthStencilDesc();
	dssDesc.StencilEnable = false;
	m_dssNoStencil = m_device.CreateDepthStencilState(dssDesc);

	dssDesc = m_device.DefaultDepthStencilDesc();
	dssDesc.StencilEnable = true;
	dssDesc.DepthEnable = true;
	dssDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	dssDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	dssDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_INCR_SAT;
	dssDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dssDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	dssDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	dssDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_DECR_SAT;
	dssDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dssDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	m_dssStencil = m_device.CreateDepthStencilState(dssDesc);
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
	m_textureEffect->SetTexture(m_wallTexture);

	//m_lightShadowEffect.reset(new LightShadowEffect(m_device, m_layout));
	//m_lightShadowEffect->SetProjMtxBuffer(m_projCB);
	//m_lightShadowEffect->SetViewMtxBuffer(m_viewCB);
	//m_lightShadowEffect->SetWorldMtxBuffer(m_worldCB);
	//m_lightShadowEffect->SetLightPosBuffer(m_lightPosCB);
	//m_lightShadowEffect->SetSurfaceColorBuffer(m_surfaceColorCB);

	m_particles.reset(new ParticleSystem(m_device, XMFLOAT3(0.f, 0.f, 0.f)));
	m_particles->SetViewMtxBuffer(m_viewCB);
	m_particles->SetProjMtxBuffer(m_projCB);
	m_particles->SetSamplerState(m_samplerWrap);

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

void Room::UpdateCamera(const XMMATRIX& view)
{
	//m_viewCB->Update(m_context, view);
	//m_cameraPosCB->Update(m_context, m_camera.GetPosition());
	XMMATRIX viewMtx[2];
	viewMtx[0] = view;
	XMVECTOR det;
	viewMtx[1] = XMMatrixInverse(&det, viewMtx[0]);
	m_viewCB->Update(m_context, viewMtx);
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


	UpdatePuma(dt);

}

void Room::DrawMirroredWorld()
{
	m_context->OMSetDepthStencilState(m_dssWrite.get(), 1);
	DrawMirror();
	m_context->OMSetDepthStencilState(m_dssTest.get(), 1);
	XMMATRIX viewMtx;
	m_camera.GetViewMatrix(viewMtx);
	XMMATRIX mirrorViewMtx = XMMatrixMultiply(m_mirrorMtx, viewMtx);
	UpdateCamera(mirrorViewMtx);
	m_context->RSSetState(m_rsCounterClockwise.get());

	DrawSun();
	DrawCylinder();
	DrawPuma();
	m_context->RSSetState(NULL);

	UpdateCamera(viewMtx);
	m_context->OMSetDepthStencilState(NULL, 0);
}

void Room::DrawMirror()
{
	m_surfaceColorCB->Update(m_context, XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f));
	m_phongEffect->Begin(m_context);
	m_worldCB->Update(m_context, m_mirror.getWorldMatrix());
	m_mirror.Render(m_context);
	m_phongEffect->End();
}

void Room::DrawSteelSheet()
{
	m_context->OMSetBlendState(m_bsAlpha.get(), nullptr, BS_MASK);
	m_context->OMSetDepthStencilState(m_dssNoWrite.get(), 0);
	m_textureEffect->SetTexture(m_steelSheetTexture);
	m_textureEffect->Begin(m_context);
	m_worldCB->Update(m_context, m_steelSheet.getWorldMatrix());
	m_steelSheet.Render(m_context);
	m_textureEffect->End();
	m_context->OMSetDepthStencilState(nullptr, 0);

	m_particles->SetViewMtxBuffer(m_viewCB);

	m_context->OMSetBlendState(nullptr, nullptr, BS_MASK);

	m_particles->Render(m_context);

}

void Room::DrawCircle()
{
	m_context->OMSetBlendState(m_bsAlpha.get(), nullptr, BS_MASK);
	m_context->OMSetDepthStencilState(m_dssNoWrite.get(), 0);
	m_surfaceColorCB->Update(m_context, XMFLOAT4(1.0f, 0.0f, 0.0f, 0.35f));
	m_phongEffect->Begin(m_context);
	m_worldCB->Update(m_context, m_circle.getWorldMatrix());
	m_circle.Render(m_context);
	m_phongEffect->End();
	m_context->OMSetDepthStencilState(nullptr, 0);
	m_context->OMSetBlendState(nullptr, nullptr, BS_MASK);
}

void Room::DrawPuma()
{
	m_surfaceColorCB->Update(m_context, XMFLOAT4(1.0f, 0.5f, 0.5f, 1.0f));
	m_phongEffect->Begin(m_context);
	m_worldCB->Update(m_context, m_puma.getWorldMatrix());
	m_puma.Render(m_context);
	m_phongEffect->End();


	m_phongEffect->Begin(m_context);
	m_surfaceColorCB->Update(m_context, XMFLOAT4(0.1f, 0.7f, 0.2f, 1.0f));

	m_worldCB->Update(m_context, m_mesh1.getWorldMatrix());
	m_mesh1.Render(m_context);

	m_worldCB->Update(m_context, m_mesh2.getWorldMatrix());
	m_mesh2.Render(m_context);

	m_worldCB->Update(m_context, m_mesh3.getWorldMatrix());
	m_mesh3.Render(m_context);

	m_worldCB->Update(m_context, m_mesh4.getWorldMatrix());
	m_mesh4.Render(m_context);

	m_worldCB->Update(m_context, m_mesh5.getWorldMatrix());
	m_mesh5.Render(m_context);

	m_worldCB->Update(m_context, m_mesh6.getWorldMatrix());
	m_mesh6.Render(m_context);

	m_surfaceColorCB->Update(m_context, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));

	m_phongEffect->End();

}

void Room::DrawCylinder()
{
	m_surfaceColorCB->Update(m_context, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	m_phongEffect->Begin(m_context);
	m_worldCB->Update(m_context, m_cylinder.getWorldMatrix());
	m_cylinder.Render(m_context);
	m_phongEffect->End();
}

void Room::DrawSun()
{
	m_textureEffect->SetTexture(m_sunTexture);
	m_textureEffect->Begin(m_context);
	m_worldCB->Update(m_context, m_sun.getWorldMatrix());
	m_sun.Render(m_context);
	m_textureEffect->End();
}

void Room::DrawWalls()
{
	m_context->OMSetBlendState(m_bsAlpha.get(), nullptr, BS_MASK);
	m_context->OMSetDepthStencilState(m_dssNoWrite.get(), 0);
	m_textureEffect->SetTexture(m_wallTexture);
	m_textureEffect->Begin(m_context);
	//for (size_t i = 0; i < 6; i++)
	//{
	//	m_worldCB->Update(m_context, m_walls[i].getWorldMatrix());
	//	m_walls[i].Render(m_context);
	//}
	m_worldCB->Update(m_context, m_walls[4].getWorldMatrix());
	m_walls[4].Render(m_context);
	m_textureEffect->End();
	m_context->OMSetDepthStencilState(nullptr, 0);
	m_context->OMSetBlendState(nullptr, nullptr, BS_MASK);
}

void Room::DrawScene()
{
	DrawMirroredWorld();
	DrawWalls();
	DrawSun();
	DrawCylinder();
	DrawPuma();
	DrawSteelSheet();
	DrawCircle();

	//UpdateCamera();

	m_particles->Render(m_context);

	m_context->RSSetState(nullptr);
}

void Room::DrawShadowVolumes()
{
	for (size_t i = 0; i < 6; i++)
	{
		m_surfaceColorCB->Update(m_context, XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f));
		m_phongEffect->Begin(m_context);
		m_worldCB->Update(m_context, m_shadowVolumes[i].getWorldMatrix());
		m_shadowVolumes[i].Render(m_context);
		m_phongEffect->End();
	}
}

void Room::DrawShadowScene()
{
	//m_context->OMSetRenderTargets(0, NULL, m_depthStencilView.get());
	m_context->OMSetBlendState(m_bsNone.get(), nullptr, BS_MASK);
	DrawScene();
	m_context->RSSetState(m_rsCullBack.get());
	m_context->OMSetDepthStencilState(m_dssIncr.get(), 0);
	DrawShadowVolumes();
	m_context->RSSetState(m_rsCullFront.get());
	m_context->OMSetDepthStencilState(m_dssDecr.get(), 0);
	DrawShadowVolumes();
	//ResetRenderTarget();
	m_context->OMSetBlendState(m_bsAll.get(), nullptr, BS_MASK);
	m_context->RSSetState(m_rsCullBack.get());
	m_context->OMSetDepthStencilState(m_dssKeepGreather.get(), 0);
	//disable lights
	DrawScene();
	m_context->OMSetDepthStencilState(m_dssKeepEqual.get(), 0);
	//enable lights
	DrawScene();
	m_context->OMSetDepthStencilState(m_dssNoStencil.get(), 0);
	m_context->OMSetBlendState(m_bsNone.get(), nullptr, BS_MASK);
	m_context->RSSetState(m_rsCullNone.get());
	m_context->OMSetDepthStencilState(m_dssStencil.get(), 0);
	DrawShadowVolumes();
	m_context->OMSetBlendState(m_bsAll.get(), nullptr, BS_MASK);
}

void Room::Render()
{
	if (m_context == nullptr)
		return;

	ResetRenderTarget();
	m_projCB->Update(m_context, m_projMtx);
	UpdateCamera();

	//Clear buffers
	float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	m_context->ClearRenderTargetView(m_backBuffer.get(), clearColor);
	m_context->ClearDepthStencilView(m_depthStencilView.get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	DrawShadowScene();
	DrawScene();

	m_swapChain->Present(0, 0);
}
