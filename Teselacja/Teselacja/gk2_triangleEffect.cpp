#include "gk2_triangleEffect.h"
#include "gk2_vertices.h"

using namespace std;
using namespace gk2;

const wstring TriangleEffect::ShaderFile = L"resources/shaders/TessellatedTriangle.hlsl";

TriangleEffect::TriangleEffect(gk2::DeviceHelper& device, std::shared_ptr<ID3D11InputLayout>& layout,
					   std::shared_ptr<ID3D11DeviceContext> context /* = nullptr */)
	: EffectBase(context)
{
	Initialize(device, layout, ShaderFile);
}

void TriangleEffect::SetSurfaceColorBuffer(const std::shared_ptr<gk2::ConstantBuffer<XMFLOAT4>>& surfaceColor)
{
	if (surfaceColor != nullptr)
		m_surfaceColorCB = surfaceColor;
}

void TriangleEffect::SetVertexShaderData()
{
	ID3D11Buffer* vsb[2] = { m_worldCB->getBufferObject().get(), m_viewCB->getBufferObject().get() };
	m_context->VSSetConstantBuffers(0, 2, vsb);
}

void TriangleEffect::SetHullShaderData()
{

}

void TriangleEffect::SetDomainShaderData()
{
	ID3D11Buffer* dsb[1] = { m_projCB->getBufferObject().get() };
	m_context->DSSetConstantBuffers(0, 1, dsb);
}

void TriangleEffect::SetPixelShaderData()
{
	ID3D11Buffer* psb[1] = { m_surfaceColorCB->getBufferObject().get() };
	m_context->PSSetConstantBuffers(0, 1, psb);
}