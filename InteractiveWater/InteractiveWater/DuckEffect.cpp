#include "gk2_duckEffect.h"

using namespace std;
using namespace gk2;


const wstring DuckEffect::ShaderFile = L"resources/shaders/DuckShader.hlsl";

DuckEffect::DuckEffect(DeviceHelper& device, shared_ptr<ID3D11InputLayout>& layout,
	shared_ptr<ID3D11DeviceContext> context /* = nullptr */)
	: EffectBase(context)
{
	Initialize(device, layout, ShaderFile);
}

void DuckEffect::SetTextureMtxBuffer(const shared_ptr<gk2::CBMatrix>& textureMtx)
{
	if (textureMtx != nullptr)
		m_textureMtxCB = textureMtx;
}

void DuckEffect::SetSamplerState(const shared_ptr<ID3D11SamplerState>& samplerState)
{
	if (samplerState != nullptr)
		m_samplerState = samplerState;
}

void DuckEffect::SetTexture(const shared_ptr<ID3D11ShaderResourceView>& texture)
{
	if (texture != nullptr)
		m_texture = texture;
}

void DuckEffect::SetCoordinates(const shared_ptr<ID3D11ShaderResourceView>& coordinates)
{
	if (coordinates != nullptr)
		m_coordinates = coordinates;
}

void DuckEffect::SetVertexShaderData()
{
	ID3D11Buffer* vsb[4] = { m_worldCB->getBufferObject().get(), m_viewCB->getBufferObject().get(),
		m_projCB->getBufferObject().get(), m_textureMtxCB->getBufferObject().get() };
	m_context->VSSetConstantBuffers(0, 4, vsb);
}

void DuckEffect::SetPixelShaderData()
{
	ID3D11SamplerState* ss[1] = { m_samplerState.get() };
	m_context->PSSetSamplers(0, 1, ss);
	ID3D11ShaderResourceView* srv[1] = { m_texture.get() };
	m_context->PSSetShaderResources(0, 1, srv);
}