#ifndef __GK2_TESSELLATION_H_
#define __GK2_TESSELLATION_H_

#include "gk2_applicationBase.h"
#include "gk2_camera.h"
#include "gk2_constantBuffer.h"
#include "gk2_triangleEffect.h"

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

		std::shared_ptr<ID3D11Buffer> m_vertexBuffer;
		unsigned int m_vertexStride;
		unsigned int m_vertexCount;

		XMMATRIX m_projMtx;
		gk2::Camera m_camera;

		std::shared_ptr<gk2::CBMatrix> m_worldCB;
		std::shared_ptr<gk2::CBMatrix> m_viewCB;
		std::shared_ptr<gk2::CBMatrix> m_projCB;
		std::shared_ptr<gk2::ConstantBuffer<XMFLOAT4>> m_surfaceColorCB;

		std::shared_ptr<gk2::TriangleEffect> m_triangleEffect;
		std::shared_ptr<ID3D11InputLayout> m_layout;

		std::shared_ptr<ID3D11RasterizerState> m_rsWireframe;

		void InitializeCamera();
		void InitializeConstantBuffers();
		void InitializeRenderStates();

		void UpdateCamera();

		void DrawScene();
	};
}

#endif __GK2_TESSELLATION_H_