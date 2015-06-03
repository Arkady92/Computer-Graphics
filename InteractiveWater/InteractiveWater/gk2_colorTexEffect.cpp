#include "gk2_colorTexEffect.h"
#include "gk2_vertices.h"

using namespace std;
using namespace gk2;


const wstring ColorTexEffect::ShaderFile = L"resources/shaders/ColorTexShader.hlsl";

ColorTexEffect::ColorTexEffect(DeviceHelper& device, shared_ptr<ID3D11InputLayout>& layout,
	shared_ptr<ID3D11DeviceContext> context /* = nullptr */)
	: EffectBase(context)
{
	Initialize(device, layout, ShaderFile);
}

void ColorTexEffect::SetTextureMtxBuffer(const shared_ptr<gk2::CBMatrix>& textureMtx)
{
	if (textureMtx != nullptr)
		m_textureMtxCB = textureMtx;
}

void ColorTexEffect::SetSamplerState(const shared_ptr<ID3D11SamplerState>& samplerState)
{
	if (samplerState != nullptr)
		m_samplerState = samplerState;
}

void ColorTexEffect::SetTexture(const shared_ptr<ID3D11ShaderResourceView>& texture)
{
	if (texture != nullptr)
		m_texture = texture;
}

void ColorTexEffect::SetTextureA(const shared_ptr<ID3D11ShaderResourceView>& texture)
{
	if (texture != nullptr)
		m_textureA = texture;
}

void ColorTexEffect::SetSurfaceColorBuffer(const shared_ptr<ConstantBuffer<XMFLOAT4>>& surfaceColor)
{
	if (surfaceColor != nullptr)
		m_surfaceColorCB = surfaceColor;
}

void ColorTexEffect::SetVertexShaderData()
{
	ID3D11Buffer* vsb[4] = { m_worldCB->getBufferObject().get(), m_viewCB->getBufferObject().get(),
		m_projCB->getBufferObject().get(), m_textureMtxCB->getBufferObject().get() };
	m_context->VSSetConstantBuffers(0, 4, vsb);
}

void ColorTexEffect::SetPixelShaderData()
{
	ID3D11Buffer* psb[1] = { m_surfaceColorCB->getBufferObject().get() };
	m_context->PSSetConstantBuffers(0, 1, psb);
	ID3D11SamplerState* ss[1] = { m_samplerState.get() };
	m_context->PSSetSamplers(0, 1, ss);
	ID3D11ShaderResourceView* srv[2] = { m_texture.get(), m_textureA.get() };
	m_context->PSSetShaderResources(0, 2, srv);
}

void ColorTexEffect::Initialize(DeviceHelper& device, shared_ptr<ID3D11InputLayout>& layout, const wstring& shaderFile)
{
	shared_ptr<ID3DBlob> vsByteCode = device.CompileD3DShader(shaderFile, "VS_Main", "vs_4_0");
	shared_ptr<ID3DBlob> psByteCode = device.CompileD3DShader(shaderFile, "PS_Main", "ps_4_0");
	m_vs = device.CreateVertexShader(vsByteCode);
	m_ps = device.CreatePixelShader(psByteCode);
	if (layout == nullptr)
	{
		m_layout = device.CreateInputLayout<VertexPosNormalCoord>(vsByteCode);
		layout = m_layout;
	}
	else
		m_layout = layout;
}