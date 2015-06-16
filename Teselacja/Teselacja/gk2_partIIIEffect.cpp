#include "gk2_partIIIEffect.h"
#include "gk2_vertices.h"

using namespace std;
using namespace gk2;

const wstring PartIIIEffect::ShaderFile = L"resources/shaders/PartIIIShader.hlsl";

PartIIIEffect::PartIIIEffect(gk2::DeviceHelper& device, std::shared_ptr<ID3D11InputLayout>& layout,
					   std::shared_ptr<ID3D11DeviceContext> context /* = nullptr */)
	: EffectBase(context)
{
	Initialize(device, layout, ShaderFile);
}

void PartIIIEffect::SetSurfaceColorBuffer(const std::shared_ptr<gk2::ConstantBuffer<XMFLOAT4>>& surfaceColor)
{
	if (surfaceColor != nullptr)
		m_surfaceColorCB = surfaceColor;
}

void PartIIIEffect::SetVertexShaderData()
{
	ID3D11Buffer* vsb[2] = { m_worldCB->getBufferObject().get(), m_viewCB->getBufferObject().get() };
	m_context->VSSetConstantBuffers(0, 2, vsb);
}

void PartIIIEffect::SetHullShaderData()
{
	ID3D11Buffer* hsb[2] = { m_edgeTessellationFactor->getBufferObject().get(), m_interiorTessellationFactor->getBufferObject().get() };
	m_context->HSSetConstantBuffers(0, 2, hsb);
}

void PartIIIEffect::SetDomainShaderData()
{
	ID3D11Buffer* dsb[1] = { m_projCB->getBufferObject().get() };
	m_context->DSSetConstantBuffers(0, 1, dsb);
}

void PartIIIEffect::SetPixelShaderData()
{
	ID3D11Buffer* psb[1] = { m_surfaceColorCB->getBufferObject().get() };
	m_context->PSSetConstantBuffers(0, 1, psb);
}