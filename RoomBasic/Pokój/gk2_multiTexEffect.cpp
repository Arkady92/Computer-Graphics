#include "gk2_multiTexEffect.h"

using namespace std;
using namespace gk2;


const wstring MultiTextureEffect::ShaderFile = L"resources/shaders/MultiTextureShader.hlsl";

MultiTextureEffect::MultiTextureEffect(DeviceHelper& device, shared_ptr<ID3D11InputLayout>& layout,
	shared_ptr<ID3D11DeviceContext> context /* = nullptr */)
	: EffectBase(context)
{
	Initialize(device, layout, ShaderFile);
}

void MultiTextureEffect::SetTextureMtxBuffer(const shared_ptr<gk2::CBMatrix>& textureMtx)
{
	if (textureMtx != nullptr)
		m_textureMtxCB = textureMtx;
}

void MultiTextureEffect::SetTextureMtxBuffer2(const shared_ptr<gk2::CBMatrix>& textureMtx)
{
	if (textureMtx != nullptr)
		m_textureMtxCB2 = textureMtx;
}

void MultiTextureEffect::SetSamplerState(const shared_ptr<ID3D11SamplerState>& samplerState)
{
	if (samplerState != nullptr)
		m_samplerState = samplerState;
}

void MultiTextureEffect::SetTexture(const shared_ptr<ID3D11ShaderResourceView>& texture)
{
	if (texture != nullptr)
		m_texture = texture;
}

void MultiTextureEffect::SetTexture2(const shared_ptr<ID3D11ShaderResourceView>& texture)
{
	if (texture != nullptr)
		m_texture2 = texture;
}

void MultiTextureEffect::SetVertexShaderData()
{
	ID3D11Buffer* vsb[5] = { m_worldCB->getBufferObject().get(), m_viewCB->getBufferObject().get(),
		m_projCB->getBufferObject().get(), m_textureMtxCB->getBufferObject().get(), m_textureMtxCB2->getBufferObject().get() };
	m_context->VSSetConstantBuffers(0, 5, vsb);
}

void MultiTextureEffect::SetPixelShaderData()
{
	ID3D11SamplerState* ss[1] = { m_samplerState.get() };
	m_context->PSSetSamplers(0, 1, ss);
	ID3D11ShaderResourceView* srv[2] = { m_texture.get(), m_texture2.get() };
	m_context->PSSetShaderResources(0, 2, srv);
}