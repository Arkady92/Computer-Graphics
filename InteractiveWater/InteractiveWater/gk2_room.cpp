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
	m_textureCB2.reset(new CBMatrix(m_device));
	m_colorTextureCB.reset(new CBMatrix(m_device));
	m_lightPosCB.reset(new ConstantBuffer<XMFLOAT4>(m_device));
	m_surfaceColorCB.reset(new ConstantBuffer<XMFLOAT4>(m_device));
	m_cameraPosCB.reset(new ConstantBuffer<XMFLOAT4>(m_device));
}

void Room::InitializeTextures()
{
	m_cubicMapTexture = m_device.CreateShaderResourceView(L"resources/textures/cubic_map.png");
	m_duckTexture = m_device.CreateShaderResourceView(L"resources/textures/ducktex.jpg");
	m_anisotrophyTexture = m_device.CreateShaderResourceView(L"resources/textures/Anisotrophy.png");
	m_waterBaseTexture = m_device.CreateShaderResourceView(L"resources/textures/bottom.jpg");
	m_skyTexture = m_device.CreateShaderResourceView(L"resources/textures/sky.png");
	m_bottomTexture = m_device.CreateShaderResourceView(L"resources/textures/bottom.jpg");
	m_forestTexture = m_device.CreateShaderResourceView(L"resources/textures/forest.jpg");

	D3D11_SAMPLER_DESC sd = m_device.DefaultSamplerDesc();
	sd.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sd.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	m_samplerWrap = m_device.CreateSamplerState(sd);

	D3D11_TEXTURE2D_DESC texDesc = m_device.DefaultTexture2DDesc();
	texDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
	texDesc.Width = N;
	texDesc.Height = N;
	m_renderTexture = m_device.CreateTexture2D(texDesc);

	currentHeightsArray = std::shared_ptr<FLOAT>(new FLOAT[N * N], Utils::DeleteArray<FLOAT>);
	previousHeightsArray = std::shared_ptr<FLOAT>(new FLOAT[N * N], Utils::DeleteArray<FLOAT>);
	suppressionsArray = std::shared_ptr<FLOAT>(new FLOAT[N * N], Utils::DeleteArray<FLOAT>);
	FLOAT *cHA = currentHeightsArray.get();
	FLOAT *pHA = previousHeightsArray.get();
	FLOAT *sA = suppressionsArray.get();

	float l = 0.0f;
	for (int i = 0; i < N; i++)
	{
		for (int j = 0; j < 256; j++)
		{
			l = max(min(min(min(i, j), min(256 - i, 256 - j)) / 256.0f / 2, 0.18f), 0.16f);
			cHA[i * N + j] = 0;
			pHA[i * N + j] = 0;
			sA[i * N + j] = 0.95f * min(1.0f, l / 0.2f);
		}
	}
}

void Room::InitializeCamera()
{
	SIZE s = getMainWindow()->getClientSize();
	float ar = static_cast<float>(s.cx) / s.cy;
	m_projMtx = XMMatrixPerspectiveFovLH(XM_PIDIV4, ar, 0.01f, 100.0f);
	m_projCB->Update(m_context, m_projMtx);
	m_camera.MoveCamera(8.5f, 0.0f, 0.0f);
	UpdateCamera();
}

void Room::CreateScene()
{
	// walls
	m_walls[0] = m_meshLoader.GetQuad(2.0f);
	for (int i = 1; i < 6; ++i)
		m_walls[i] = m_walls[0];
	XMMATRIX wall = XMMatrixTranslation(0.0f, 0.0f, 1.0f);
	float a = 0;
	for (int i = 0; i < 4; ++i, a += XM_PIDIV2)
		m_walls[i].setWorldMatrix(wall * XMMatrixRotationY(a));
	m_walls[4].setWorldMatrix(wall * XMMatrixRotationX(XM_PIDIV2));
	m_walls[5].setWorldMatrix(wall * XMMatrixRotationX(-XM_PIDIV2));

	// duck
	m_duck = m_meshLoader.LoadMeshForDuck(L"resources/meshes/duck.txt");
	m_duck.setWorldMatrix(XMMatrixScaling(0.002f, 0.002f, 0.002f) * XMMatrixTranslation(0.0f, 0.0f, 0.0f));
	baseDuckMatrix = m_duck.getWorldMatrix();

	// water
	m_water = m_meshLoader.GetQuad(2.0f, 2.0f);
	m_water.setWorldMatrix(XMMatrixRotationX(XM_PIDIV2) * XMMatrixTranslation(0.0f, -0.5f, 0.0f));

	// b-spline
	deBoorsPoints = vector<XMFLOAT3>(4);
	deBoorsPoints[0] = XMFLOAT3(-0.5f, -0.5f, -0.5f);
	deBoorsPoints[1] = XMFLOAT3(-0.5f, -0.5f, 0.5f);
	deBoorsPoints[2] = XMFLOAT3(0.5f, -0.5f, -0.5f);
	deBoorsPoints[3] = XMFLOAT3(0.5f, -0.5f, 0.5f);

	m_lightPosCB->Update(m_context, LIGHT_POS);
	m_textureCB->Update(m_context, XMMatrixScaling(0.5f, 0.5f, 0.5f) * XMMatrixTranslation(0.5f, 0.5f, 0.0f) * XMMatrixRotationZ(XM_PI));
	m_textureCB2->Update(m_context, XMMatrixScaling(0.5f, 0.5f, 0.5f) * XMMatrixTranslation(0.5f, 0.5f, 0.0f) * XMMatrixRotationZ(XM_PI));
	m_colorTextureCB->Update(m_context, XMMatrixScaling(0.5f, 0.5f, 0.5f) * XMMatrixRotationX(XM_PIDIV4));
}

void Room::UpdateDuck(float dt)
{
	const int factor[] = { 1, -1 };
	duckPositionParameter += dt * duckStepFactor;
	if (duckPositionParameter >= 3.0f / 5.0f)
	{
		deBoorsPoints[0] = deBoorsPoints[1];
		deBoorsPoints[1] = deBoorsPoints[2];
		deBoorsPoints[2] = deBoorsPoints[3];

		deBoorsPoints[3] = XMFLOAT3(factor[rand() % 2] * (rand() % 50 + 50) / 100.0f, -0.5f, factor[rand() % 2] * (rand() % 100 - 50) / 100.0f);
		duckPositionParameter = 2.0f / 5.0f;
	}
	XMFLOAT3 currentDuckPosition = GetDuckPosition(duckPositionParameter);
	if (abs(duckPosition.x - currentDuckPosition.x) < 0.001 && abs(duckPosition.z - currentDuckPosition.z) < 0.001) return;
	float alpha = atan2((currentDuckPosition.z - duckPosition.z), (currentDuckPosition.x - duckPosition.x));
	m_duck.setWorldMatrix(baseDuckMatrix * XMMatrixRotationY(XM_PI - alpha) *
		XMMatrixTranslation(duckPosition.x, duckPosition.y, duckPosition.z));
	duckPosition = currentDuckPosition;
}

XMFLOAT3 Room::GetDuckPosition(float t)
{
	int knotsDivider = 5;
	float knotsDivision = 1.0 / knotsDivider;
	vector<double> knots(8);
	knots[0] = 0;
	for (int i = 1; i < 7; i++)
		knots[i] = (i - 1) * knotsDivision;
	knots[7] = 1;
	XMFLOAT3 position(0.0f, 0.0f, 0.0f);
	for (int i = 0; i < 4; i++)
	{
		float result = CalculateNSplineValue(knots, i + 1, 3, t);
		position.x += deBoorsPoints[i].x * result;
		position.y += deBoorsPoints[i].y * result;
		position.z += deBoorsPoints[i].z * result;
	}
	return position;
}

XMFLOAT3 Room::Cross(XMFLOAT3 a, XMFLOAT3 b)
{
	XMFLOAT3 cross = XMFLOAT3(a.y * b.z - b.y * a.z,
		a.z * b.x - b.z * a.x,
		a.x * b.y - b.x * a.y);
	float length = sqrt(cross.x * cross.x + cross.y * cross.y + cross.z * cross.z);
	return XMFLOAT3(cross.x / length, cross.y / length, cross.z / length);
}

double Room::CalculateZeroSplineValue(vector<double> knots, int i, double t)
{
	double result = 0.0;
	if (i <= 0 || i >= knots.size()) return result;
	if (knots[i - 1] <= t && knots[i] > t)
		result = 1;
	return result;
}

double Room::CalculateNSplineValue(vector<double> knots, int i, int n, double t)
{
	if (n == 0)
		return CalculateZeroSplineValue(knots, i, t);
	double result = 0.0;
	if (i > 0 && i + n - 1 < knots.size())
	{
		double fun = CalculateNSplineValue(knots, i, n - 1, t);
		double factor = knots[i + n - 1] - knots[i - 1];
		if (abs(factor) > 0)
			result += (t - knots[i - 1]) / factor * fun;
	}
	if (i >= 0 && i + n < knots.size() - 1)
	{
		double fun = CalculateNSplineValue(knots, i + 1, n - 1, t);
		double factor = knots[i + n] - knots[i];
		if (abs(factor) > 0)
			result += (knots[i + n] - t) / factor * fun;
	}
	return result;
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

	m_colorTextureEffect.reset(new ColorTexEffect(m_device, m_layout));
	m_colorTextureEffect->SetProjMtxBuffer(m_projCB);
	m_colorTextureEffect->SetViewMtxBuffer(m_viewCB);
	m_colorTextureEffect->SetWorldMtxBuffer(m_worldCB);
	m_colorTextureEffect->SetTextureMtxBuffer(m_colorTextureCB);
	m_colorTextureEffect->SetSamplerState(m_samplerWrap);
	m_colorTextureEffect->SetTexture(m_duckTexture);
	m_colorTextureEffect->SetTextureA(m_anisotrophyTexture);
	m_colorTextureEffect->SetSurfaceColorBuffer(m_surfaceColorCB);

	m_phongEffect.reset(new PhongEffect(m_device, m_layout));
	m_phongEffect->SetProjMtxBuffer(m_projCB);
	m_phongEffect->SetViewMtxBuffer(m_viewCB);
	m_phongEffect->SetWorldMtxBuffer(m_worldCB);
	m_phongEffect->SetLightPosBuffer(m_lightPosCB);
	m_phongEffect->SetSurfaceColorBuffer(m_surfaceColorCB);

	m_multiTextureEffect.reset(new MultiTexEffect(m_device, m_layout));
	m_multiTextureEffect->SetProjMtxBuffer(m_projCB);
	m_multiTextureEffect->SetViewMtxBuffer(m_viewCB);
	m_multiTextureEffect->SetWorldMtxBuffer(m_worldCB);
	m_multiTextureEffect->Set1stTextureMtxBuffer(m_textureCB);
	m_multiTextureEffect->Set2ndTextureMtxBuffer(m_textureCB2);
	m_multiTextureEffect->SetSamplerState(m_samplerWrap);
	m_multiTextureEffect->Set1stTexture(m_waterBaseTexture);
	m_multiTextureEffect->Set2ndTexture(m_waterTexture);

	m_textureEffect.reset(new TextureEffect(m_device, m_layout));
	m_textureEffect->SetProjMtxBuffer(m_projCB);
	m_textureEffect->SetViewMtxBuffer(m_viewCB);
	m_textureEffect->SetWorldMtxBuffer(m_worldCB);
	m_textureEffect->SetTextureMtxBuffer(m_textureCB);
	m_textureEffect->SetSamplerState(m_samplerWrap);

	m_cubeMapper.reset(new CubeMapper(m_device, m_layout, m_context, 0.4f, 8.0f,
		XMFLOAT3(-1.3f, -0.74f, -0.6f)));
	m_cubeMapper->SetProjMtxBuffer(m_projCB);
	m_cubeMapper->SetViewMtxBuffer(m_viewCB);
	m_cubeMapper->SetWorldMtxBuffer(m_worldCB);
	m_cubeMapper->SetSamplerState(m_samplerWrap);
	m_cubeMapper->SetCameraPosBuffer(m_cameraPosCB);
	m_cubeMapper->SetSurfaceColorBuffer(m_surfaceColorCB);

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
	const float factor = 1 / 300.0f;
	if (m_keyboard->GetState(currentKeyboardState)){

		if (currentKeyboardState.isKeyDown(DIK_Q) || currentKeyboardState.isKeyDown(DIK_LSHIFT)){
			m_camera.MoveCamera(0, 0, factor);
		}

		if (currentKeyboardState.isKeyDown(DIK_Z) || currentKeyboardState.isKeyDown(DIK_LCONTROL)){
			m_camera.MoveCamera(0, 0, -factor);
		}

		if (currentKeyboardState.isKeyDown(DIK_W) || currentKeyboardState.isKeyDown(DIK_UP)){
			m_camera.MoveCamera(factor, 0, 0);
		}

		if (currentKeyboardState.isKeyDown(DIK_S) || currentKeyboardState.isKeyDown(DIK_DOWN)){
			m_camera.MoveCamera(-factor, 0, 0);
		}

		if (currentKeyboardState.isKeyDown(DIK_A) || currentKeyboardState.isKeyDown(DIK_LEFT)){
			m_camera.MoveCamera(0, factor, 0);
		}

		if (currentKeyboardState.isKeyDown(DIK_D) || currentKeyboardState.isKeyDown(DIK_RIGHT)){
			m_camera.MoveCamera(0, -factor, 0);
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
	UpdateWater(dt);
}

void Room::UpdateWater(float dt)
{
	if (rand() % 10 != 0) return;
	shared_ptr<FLOAT> tempData(new FLOAT[N * N], Utils::DeleteArray<FLOAT>);
	FLOAT *tD = tempData.get();
	FLOAT *cHA = currentHeightsArray.get();
	FLOAT *pHA = previousHeightsArray.get();
	FLOAT *sA = suppressionsArray.get();

	cHA[(int)((-duckPosition.z + 1) * N / 2) * N + (int)((-duckPosition.x + 1) * N / 2)] = 0.25f;
	if (rand() % 10 == 0) cHA[(rand() % 156 + 50) * N + (rand() % 156 + 50)] = 0.25f;

	for (int i = 0; i < N; ++i)
	{
		for (int j = 0; j < N; ++j)
		{
			float A = (c * c * dT * dT) / (h*h);
			float B = 2.0f - 4.0f * A;
			if (A > 0.5f) tD[i * 256 + j] = 0.0f;
			float z = 0.0f;
			if (i + 1 < N) z += cHA[(i + 1) * N + j];
			if (i - 1 >= 0) z += cHA[(i - 1) * N + j];
			if (j + 1 < N) z += cHA[i * N + (j + 1)];
			if (j - 1 >= 0) z += cHA[i * N + (j - 1)];
			z *= A;
			z += B * cHA[i * N + j];
			z -= pHA[i * N + j];
			z *= sA[i * N + j];

			tD[i * 256 + j] = z;
		}
	}
	for (int i = 0; i < 256; ++i)
	{
		for (int j = 0; j < 256; ++j)
		{
			pHA[i * N + j] = cHA[i * N + j];
			cHA[i * N + j] = tD[i * 256 + j];
		}
	}

	shared_ptr<BYTE> normals(new BYTE[N * N * 4], Utils::DeleteArray<BYTE>);
	BYTE *n = normals.get();

	for (int i = 0; i < N; ++i)
	{
		for (int j = 0; j < N; ++j)
		{
			XMFLOAT3 normal = Cross(XMFLOAT3(h, cHA[i * N + j + 1] - cHA[i * N + j], 0), 
				XMFLOAT3(0, cHA[(i - 1) * N + j] - cHA[i * N + j], -h));
			n[i * N * 4 + j * 4] = normal.x * 255;
			n[i * N * 4 + j * 4 + 1] = normal.y * 255;
			n[i * N * 4 + j * 4 + 2] = normal.z * 255;
			n[i * N * 4 + j * 4 + 3] = 255.0f;
		}
	}

	m_context->UpdateSubresource(m_renderTexture.get(), 0, 0, normals.get(), N * 4, N * N * 4);
	m_waterTexture = m_device.CreateShaderResourceView(m_renderTexture);
}

void Room::DrawWalls()
{
	m_textureEffect->SetTexture(m_forestTexture);
	m_textureEffect->Begin(m_context);
	//m_cubeMapper->Begin(m_context);
	for (size_t i = 0; i < 4; i++)
	{
		m_worldCB->Update(m_context, m_walls[i].getWorldMatrix());
		m_walls[i].Render(m_context);
	}
	//m_cubeMapper->End();
	m_textureEffect->End();

	m_textureEffect->SetTexture(m_bottomTexture);
	m_textureEffect->Begin(m_context);
	m_worldCB->Update(m_context, m_walls[4].getWorldMatrix());
	m_walls[4].Render(m_context);
	m_textureEffect->End();

	m_textureEffect->SetTexture(m_skyTexture);
	m_textureEffect->Begin(m_context);
	m_worldCB->Update(m_context, m_walls[5].getWorldMatrix());
	m_walls[5].Render(m_context);
	m_textureEffect->End();
}

void Room::DrawDuck()
{
	m_colorTextureEffect->SetTexture(m_duckTexture);
	m_colorTextureEffect->Begin(m_context);
	m_worldCB->Update(m_context, m_duck.getWorldMatrix());
	m_duck.Render(m_context);
	m_colorTextureEffect->End();
}

void Room::DrawWater()
{
	m_context->OMSetBlendState(m_bsAlpha.get(), nullptr, BS_MASK);
	m_context->OMSetDepthStencilState(m_dssNoWrite.get(), 0);
	m_multiTextureEffect->Set2ndTexture(m_waterTexture);
	m_multiTextureEffect->Begin(m_context);
	m_worldCB->Update(m_context, m_water.getWorldMatrix());
	m_water.Render(m_context);
	m_multiTextureEffect->End();
	m_context->OMSetDepthStencilState(nullptr, 0);
	m_context->OMSetBlendState(nullptr, nullptr, BS_MASK);
}

void Room::DrawScene()
{
	DrawDuck();
	DrawWalls();
	DrawWater();

	m_context->RSSetState(nullptr);
}

void Room::Render()
{
	if (m_context == nullptr)
		return;

	//auto mapper = m_cubeMapper.get();

	//for (size_t i = 0; i < 6; i++)
	//{
	//	mapper->SetupFace(m_context, (D3D11_TEXTURECUBE_FACE)i);

	//	DrawScene();
	//	mapper->EndFace();
	//}

	ResetRenderTarget();
	m_projCB->Update(m_context, m_projMtx);
	UpdateCamera();

	float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	m_context->ClearRenderTargetView(m_backBuffer.get(), clearColor);
	m_context->ClearDepthStencilView(m_depthStencilView.get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	DrawScene();

	m_swapChain->Present(0, 0);
}
