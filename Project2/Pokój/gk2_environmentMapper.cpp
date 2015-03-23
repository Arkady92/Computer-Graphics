#include "gk2_environmentMapper.h"

using namespace std;
using namespace gk2;

const wstring EnvironmentMapper::ShaderFile = L"resources/shaders/EnvMapShader.hlsl";
const int EnvironmentMapper::TEXTURE_SIZE = 256;

EnvironmentMapper::EnvironmentMapper(DeviceHelper& device, shared_ptr<ID3D11InputLayout>& layout,
	const shared_ptr<ID3D11DeviceContext>& context, float nearPlane, float farPlane, XMFLOAT3 pos)
	: EffectBase(context)
{
	Initialize(device, layout, ShaderFile);
	m_nearPlane = nearPlane;
	m_farPlane = farPlane;
	m_position = XMFLOAT4(pos.x, pos.y, pos.z, 1.0f);
	m_face = (D3D11_TEXTURECUBE_FACE)-1;
	InitializeTextures(device);
}

void EnvironmentMapper::InitializeTextures(DeviceHelper& device)
{
	//TODO: Create description for empty texture used as face render target
	
	//TODO: setup texture's width, height, mipLevels and bindflags

	//TODO: create texture

	//TODO: create render target view for that texture

	//TODO: create texture for depth buffer with appropriate size

	//TODO: create depth-stencil view for that texture

	//TODO: Create description for empty texture used as environment cube map
	//TODO: setup texture's width, height, mipLevels, bindflags, array size and miscFlags

	//TODO: Create environment cube map texture

	//TODO: Create description of shader resource view for cube map

	//TODO: Create shader resource view for cube map

}

void EnvironmentMapper::SetupFace(const shared_ptr<ID3D11DeviceContext>& context, D3D11_TEXTURECUBE_FACE face)
{
	if (context != nullptr && context != m_context)
		m_context = context;
	if (m_context == nullptr)
		return;
	//TODO: Setup view and proj matrices

	//TODO: Setup viewport

	//TODO: Bind render target view and depth stencil view and clear them

}

void EnvironmentMapper::EndFace()
{
	if (m_face < 0 || m_face > 5)
		return;
	//TODO: copy face texture to the environment cube map

}

void EnvironmentMapper::SetSamplerState(const shared_ptr<ID3D11SamplerState>& samplerState)
{
	if (samplerState != nullptr)
		m_samplerState = samplerState;
}

void EnvironmentMapper::SetCameraPosBuffer(const shared_ptr<ConstantBuffer<XMFLOAT4>>& cameraPos)
{
	if (cameraPos != nullptr)
		m_cameraPosCB = cameraPos;
}

void EnvironmentMapper::SetSurfaceColorBuffer(const shared_ptr<ConstantBuffer<XMFLOAT4>>& surfaceColor)
{
	if (surfaceColor != nullptr)
		m_surfaceColorCB = surfaceColor;
}

void EnvironmentMapper::SetVertexShaderData()
{
	ID3D11Buffer* vsb[4] = { m_worldCB->getBufferObject().get(), m_viewCB->getBufferObject().get(),
							 m_projCB->getBufferObject().get(),  m_cameraPosCB->getBufferObject().get() };
	m_context->VSSetConstantBuffers(0, 4, vsb);
}

void EnvironmentMapper::SetPixelShaderData()
{
	ID3D11Buffer* psb[1] = { m_surfaceColorCB->getBufferObject().get() };
	m_context->PSSetConstantBuffers(0, 1, psb);
	ID3D11SamplerState* ss[1] = { m_samplerState.get() };
	m_context->PSSetSamplers(0, 1, ss);
	ID3D11ShaderResourceView* srv[1] = { m_envTextureView.get() };
	m_context->PSSetShaderResources(0, 1, srv);
}