#ifndef __GK2_PARTIVV_EFFECT_H_
#define __GK2_PARTIVV_EFFECT_H_

#include "gk2_effectBase.h"

namespace gk2
{
	class PartIVVEffect : public gk2::EffectBase
	{
	public:
		PartIVVEffect(gk2::DeviceHelper& device, std::shared_ptr<ID3D11InputLayout>& layout,
			std::shared_ptr<ID3D11DeviceContext> context = nullptr);
		void SetSurfaceColorBuffer(const std::shared_ptr<gk2::ConstantBuffer<XMFLOAT4>>& surfaceColor);
		void SetIndexBuffer(const std::shared_ptr<gk2::ConstantBuffer<INT>>& index);
		void SetCameraPosBuffer(const std::shared_ptr<ConstantBuffer<XMFLOAT4>>& cameraPos);
		void SetSamplerState(const std::shared_ptr<ID3D11SamplerState>& samplerState, const std::shared_ptr<ID3D11SamplerState>& samplerState2);
		void SetDisplacementTexture(const std::shared_ptr<ID3D11ShaderResourceView>& texture);
		void SetColorTexture(const std::shared_ptr<ID3D11ShaderResourceView>& texture);
		void SetNormalTexture(const std::shared_ptr<ID3D11ShaderResourceView>& texture);


	protected:
		virtual void SetVertexShaderData();
		virtual void SetHullShaderData();
		virtual void SetDomainShaderData();
		virtual void SetPixelShaderData();

	private:
		static const std::wstring ShaderFile;

		std::shared_ptr<gk2::ConstantBuffer<XMFLOAT4>> m_surfaceColorCB;
		std::shared_ptr<gk2::ConstantBuffer<INT>> m_patchIndexCB;
		std::shared_ptr<gk2::ConstantBuffer<XMFLOAT4>> m_cameraPosCB;
		std::shared_ptr<ID3D11SamplerState> m_samplerState;
		std::shared_ptr<ID3D11SamplerState> m_samplerState2;
		std::shared_ptr<ID3D11ShaderResourceView> m_dispTexture;
		std::shared_ptr<ID3D11ShaderResourceView> m_colorTexture;
		std::shared_ptr<ID3D11ShaderResourceView> m_normalTexture;

	};
}

#endif __GK2_PARTIVV_EFFECT_H_