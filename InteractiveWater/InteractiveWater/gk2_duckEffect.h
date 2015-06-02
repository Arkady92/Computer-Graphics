#ifndef __GK2_DUCK_EFFECT_H_
#define __GK2_DUCK_EFFECT_H_

#include "gk2_effectBase.h"

namespace gk2
{
	class DuckEffect : public EffectBase
	{
	public:
		DuckEffect(gk2::DeviceHelper& device, std::shared_ptr<ID3D11InputLayout>& layout,
			std::shared_ptr<ID3D11DeviceContext> context = nullptr);

		void SetTextureMtxBuffer(const std::shared_ptr<gk2::CBMatrix>& textureMtx);
		void SetSamplerState(const std::shared_ptr<ID3D11SamplerState>& samplerState);
		void SetTexture(const std::shared_ptr<ID3D11ShaderResourceView>& texture);
		void SetCoordinates(const std::shared_ptr<ID3D11ShaderResourceView>& coordinates);

	protected:
		virtual void SetVertexShaderData();
		virtual void SetPixelShaderData();

	private:
		static const std::wstring ShaderFile;

		std::shared_ptr<gk2::CBMatrix> m_textureMtxCB;
		std::shared_ptr<ID3D11SamplerState> m_samplerState;
		std::shared_ptr<ID3D11ShaderResourceView> m_texture;
		std::shared_ptr<ID3D11ShaderResourceView> m_coordinates;
	};
}

#endif __GK2_DUCK_EFFECT_H_