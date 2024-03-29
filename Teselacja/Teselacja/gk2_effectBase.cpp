#include "gk2_effectBase.h"
#include "gk2_vertices.h"

using namespace std;
using namespace gk2;

EffectBase::EffectBase(shared_ptr<ID3D11DeviceContext> context /* = nullptr */)
	: m_context(context)
{
}

void EffectBase::SetWorldMtxBuffer(const shared_ptr<CBMatrix>& world)
{
	if (world != nullptr)
		m_worldCB = world;
}

void EffectBase::SetViewMtxBuffer(const shared_ptr<CBMatrix>& view)
{
	if (view != nullptr)
		m_viewCB = view;
}

void EffectBase::SetProjMtxBuffer(const shared_ptr<CBMatrix>& proj)
{
	if (proj != nullptr)
		m_projCB = proj;
}

void EffectBase::SetEedgeTessellationFactorBuffer(const std::shared_ptr<gk2::ConstantBuffer<FLOAT>>& edgeTesselationFactor)
{
	if (edgeTesselationFactor != nullptr)
		m_edgeTessellationFactor = edgeTesselationFactor;
}

void EffectBase::SetInteriorTessellationFactorBuffer(const std::shared_ptr<gk2::ConstantBuffer<FLOAT>>& interiorTessellationFactor)
{
	if (interiorTessellationFactor != nullptr)
		m_interiorTessellationFactor = interiorTessellationFactor;
}


void EffectBase::Begin(std::shared_ptr<ID3D11DeviceContext> context /* = nullptr */)
{
	if (context != nullptr && context != m_context)
		m_context = context;
	if (m_context == nullptr)
		return;
	m_context->VSSetShader(m_vs.get(), nullptr, 0);
	m_context->HSSetShader(m_hs.get(), nullptr, 0);
	m_context->DSSetShader(m_ds.get(), nullptr, 0);
	m_context->GSSetShader(nullptr, nullptr, 0);
	m_context->PSSetShader(m_ps.get(), nullptr, 0);
	m_context->IASetInputLayout(m_layout.get());

	SetVertexShaderData();
	SetHullShaderData();
	SetDomainShaderData();
	SetPixelShaderData();
}

void EffectBase::End()
{

}

void EffectBase::Initialize(DeviceHelper& device, shared_ptr<ID3D11InputLayout>& layout, const wstring& shaderFile, bool phong)
{
	shared_ptr<ID3DBlob> vsByteCode = device.CompileD3DShader(shaderFile, "VS_Main", "vs_5_0");
	if (!phong)
	{
		shared_ptr<ID3DBlob> hsByteCode = device.CompileD3DShader(shaderFile, "HS_Main", "hs_5_0");
		shared_ptr<ID3DBlob> dsByteCode = device.CompileD3DShader(shaderFile, "DS_Main", "ds_5_0");
		m_hs = device.CreateHullShader(hsByteCode);
		m_ds = device.CreateDomainShader(dsByteCode);
	}
	shared_ptr<ID3DBlob> psByteCode = device.CompileD3DShader(shaderFile, "PS_Main", "ps_5_0");
	m_vs = device.CreateVertexShader(vsByteCode);
	m_ps = device.CreatePixelShader(psByteCode);
	if (layout == nullptr)
	{
		m_layout = device.CreateInputLayout<VertexPos>(vsByteCode);
		layout = m_layout;
	}
	else
		m_layout = layout;
}