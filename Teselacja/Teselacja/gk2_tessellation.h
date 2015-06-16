#ifndef __GK2_TESSELLATION_H_
#define __GK2_TESSELLATION_H_

#include "gk2_applicationBase.h"
#include "gk2_camera.h"
#include "gk2_constantBuffer.h"
#include "gk2_partIEffect.h"
#include "gk2_partIIEffect.h"
#include "gk2_partIIIEffect.h"
#include "gk2_partIVVEffect.h"
#include "gk2_colorEffect.h"
#include <vector>

namespace gk2
{
	class Tessellation : public gk2::ApplicationBase
	{
	public:
		Tessellation(HINSTANCE hInstance);
		virtual ~Tessellation();
		static void* operator new(size_t size);
		static void operator delete(void* ptr);

	protected:
		virtual bool LoadContent();
		virtual void UnloadContent();
	
		virtual void Update(float dt);
		virtual void Render();

	private:

		std::shared_ptr<ID3D11Buffer> m_quadVertexBuffer;
		std::shared_ptr<ID3D11Buffer> m_bezierPatchVertexBuffers[2];
		std::shared_ptr<ID3D11Buffer> m_bezierControlNetVertexBuffers[2];
		std::shared_ptr<ID3D11Buffer> m_multipleBezierPatchVertexBuffers[16];
		std::shared_ptr<ID3D11Buffer> m_bezierControlNetIndexBuffer;

		unsigned int m_vertexStride;
		const unsigned int m_quadVertexCount = 4;
		const unsigned int m_bezierPatchVertexCount = 16;
		const unsigned int m_bezierPatchIndicesCount = 48;

		float ETF = 1.0f;
		float ITF = 1.0f;
		bool firstPatchEnabled = true;
		bool bezierControlNetEnabled = true;
		bool wireframeEnabled = false;
		enum parts part;

		XMMATRIX m_projMtx;
		gk2::Camera m_camera;

		std::shared_ptr<gk2::CBMatrix> m_worldCB;
		std::shared_ptr<gk2::CBMatrix> m_viewCB;
		std::shared_ptr<gk2::CBMatrix> m_projCB;
		std::shared_ptr<gk2::ConstantBuffer<XMFLOAT4>> m_surfaceColorCB;
		std::shared_ptr<gk2::ConstantBuffer<XMFLOAT4>> m_cameraPosCB;
		std::shared_ptr<gk2::ConstantBuffer<INT>> m_patchIndexCB;
		std::shared_ptr<gk2::ConstantBuffer<FLOAT>> m_edgeTessellationFactorCB;
		std::shared_ptr<gk2::ConstantBuffer<FLOAT>> m_interiorTessellationFactorCB;

		std::shared_ptr<gk2::PartIEffect> m_partIEffect;
		std::shared_ptr<gk2::PartIIEffect> m_partIIEffect;
		std::shared_ptr<gk2::PartIIIEffect> m_partIIIEffect;
		std::shared_ptr<gk2::PartIVVEffect> m_partIVVEffect;
		std::shared_ptr<gk2::ColorEffect> m_colorEffect;
		std::shared_ptr<ID3D11InputLayout> m_layout;

		std::shared_ptr<ID3D11RasterizerState> m_rsWireframe;
		std::shared_ptr<ID3D11RasterizerState> m_rsSolid;
		std::shared_ptr<ID3D11SamplerState> m_domainSamplerWrap;
		std::shared_ptr<ID3D11SamplerState> m_pixelSamplerWrap;
		std::shared_ptr<ID3D11ShaderResourceView> m_displacementTexture;
		std::shared_ptr<ID3D11ShaderResourceView> m_colorTexture;
		std::shared_ptr<ID3D11ShaderResourceView> m_normalTexture;


		void InitializeCamera();
		void InitializeConstantBuffers();
		void InitializeRenderStates();
		void InitializeTextures();
		void InitializeQuadVertexBuffer();
		void InitializeBezierControlNetVertexBuffers();
		void InitializeBezierControlNetIndicesBuffer();
		void UpdateCamera();
		void DrawScene();
		void DrawPartI();
		void DrawPartII();
		void DrawPartIII();
		void DrawPartIVV();
		void DrawBezierControlNet();
	};

	enum parts
	{
		I,
		II,
		III,
		IVV
	};
}

#endif __GK2_TESSELLATION_H_