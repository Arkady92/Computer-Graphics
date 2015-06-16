#include "gk2_partIVVEffect.h"
#include "gk2_vertices.h"

using namespace std;
using namespace gk2;

const wstring PartIVVEffect::ShaderFile = L"resources/shaders/PartIVVShader.hlsl";

PartIVVEffect::PartIVVEffect(gk2::DeviceHelper& device, std::shared_ptr<ID3D11InputLayout>& layout,
					   std::shared_ptr<ID3D11DeviceContext> context /* = nullptr */)
	: EffectBase(context)
{
	Initialize(device, layout, ShaderFile);
}

void PartIVVEffect::SetSurfaceColorBuffer(const std::shared_ptr<gk2::ConstantBuffer<XMFLOAT4>>& surfaceColor)
{
	if (surfaceColor != nullptr)
		m_surfaceColorCB = surfaceColor;
}

void PartIVVEffect::SetCameraPosBuffer(const shared_ptr<ConstantBuffer<XMFLOAT4>>& cameraPos)
{
	if (cameraPos != nullptr)
		m_cameraPosCB = cameraPos;
}

void PartIVVEffect::SetSamplerState(const shared_ptr<ID3D11SamplerState>& samplerState, const shared_ptr<ID3D11SamplerState>& samplerState2)
{
	if (samplerState != nullptr)
		m_samplerState = samplerState;
	if (samplerState2 != nullptr)
		m_samplerState2 = samplerState2;
}

void PartIVVEffect::SetDisplacementTexture(const shared_ptr<ID3D11ShaderResourceView>& texture)
{
	if (texture != nullptr)
		m_dispTexture = texture;
}

void PartIVVEffect::SetColorTexture(const shared_ptr<ID3D11ShaderResourceView>& texture)
{
	if (texture != nullptr)
		m_colorTexture = texture;
}

void PartIVVEffect::SetIndexBuffer(const std::shared_ptr<gk2::ConstantBuffer<INT>>& index)
{
	if (index != nullptr)
		m_patchIndexCB = index;
}

void PartIVVEffect::SetNormalTexture(const shared_ptr<ID3D11ShaderResourceView>& texture)
{
	if (texture != nullptr)
		m_normalTexture = texture;
}

void PartIVVEffect::SetVertexShaderData()
{
	ID3D11Buffer* vsb[3] = { m_worldCB->getBufferObject().get(), m_viewCB->getBufferObject().get(),
		m_cameraPosCB->getBufferObject().get() };
	m_context->VSSetConstantBuffers(0, 3, vsb);
}

void PartIVVEffect::SetHullShaderData()
{
	ID3D11Buffer* hsb[2] = { m_edgeTessellationFactor->getBufferObject().get(), m_interiorTessellationFactor->getBufferObject().get() };
	m_context->HSSetConstantBuffers(0, 2, hsb);
}

void PartIVVEffect::SetDomainShaderData()
{
	ID3D11Buffer* dsb[2] = { m_projCB->getBufferObject().get(), m_patchIndexCB->getBufferObject().get() };
	m_context->DSSetConstantBuffers(0, 2, dsb);
	ID3D11ShaderResourceView* srv[1] = { m_dispTexture.get() };
	m_context->DSSetShaderResources(0, 1, srv);
	ID3D11SamplerState* ss[1] = { m_samplerState2.get() };
	m_context->DSSetSamplers(0, 1, ss);
}

void PartIVVEffect::SetPixelShaderData()
{
	ID3D11SamplerState* ss[1] = { m_samplerState.get() };
	m_context->PSSetSamplers(1, 1, ss);
	ID3D11ShaderResourceView* srv[2] = { m_colorTexture.get(), m_normalTexture.get()};
	m_context->PSSetShaderResources(1, 2, srv);

	ID3D11Buffer* psb[1] = { m_surfaceColorCB->getBufferObject().get() };
	m_context->PSSetConstantBuffers(0, 1, psb);
}
