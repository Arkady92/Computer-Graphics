#ifndef __GK2_COLOR_EFFECT_H_
#define __GK2_COLOR_EFFECT_H_

#include "gk2_effectBase.h"

namespace gk2
{
	class ColorEffect : public EffectBase
	{
	public:
		ColorEffect(gk2::DeviceHelper& device, std::shared_ptr<ID3D11InputLayout>& layout,
					std::shared_ptr<ID3D11DeviceContext> context = nullptr);

		void SetSurfaceColorBuffer(const std::shared_ptr<gk2::ConstantBuffer<XMFLOAT4>>& surfaceColor);

	protected:
		virtual void SetVertexShaderData();
		virtual void SetPixelShaderData();

		virtual void SetHullShaderData() {}
		virtual void SetDomainShaderData() {}

	private:
		static const std::wstring ShaderFile;

		std::shared_ptr<gk2::ConstantBuffer<XMFLOAT4>> m_surfaceColorCB;
	};
}

#endif __GK2_COLOR_EFFECT_H_
