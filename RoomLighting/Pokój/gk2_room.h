#ifndef __GK2_ROOM_H_
#define __GK2_ROOM_H_

#include "gk2_applicationBase.h"
#include "gk2_meshLoader.h"
#include "gk2_camera.h"
#include "gk2_phongEffect.h"
#include "gk2_lightShadowEffect.h"
#include "gk2_constantBuffer.h"
#include "gk2_particles.h"

namespace gk2
{
	class Room : public gk2::ApplicationBase
	{
	public:
		Room(HINSTANCE hInstance);
		virtual ~Room();

		static void* operator new(size_t size);
		static void operator delete(void* ptr);

	protected:
		virtual bool LoadContent();
		virtual void UnloadContent();

		virtual void Update(float dt);
		virtual void Render();

	private:
		static const unsigned int BS_MASK;
		static const float TABLE_H;
		static const float TABLE_TOP_H;
		static const float TABLE_R;
		static const XMFLOAT4 TABLE_POS;

		gk2::Mesh m_walls[6];
		gk2::Mesh m_teapot;
		gk2::Mesh m_debugSphere;
		gk2::Mesh m_box;
		gk2::Mesh m_lamp;
		gk2::Mesh m_chairSeat;
		gk2::Mesh m_chairBack;
		gk2::Mesh m_tableLegs[4];
		gk2::Mesh m_tableSide;
		gk2::Mesh m_tableTop;
		gk2::Mesh m_monitor;
		gk2::Mesh m_screen;

		XMMATRIX m_projMtx;

		gk2::Camera m_camera;
		gk2::MeshLoader m_meshLoader;

		std::shared_ptr<gk2::CBMatrix> m_worldCB;
		std::shared_ptr<gk2::CBMatrix> m_viewCB;
		std::shared_ptr<gk2::CBMatrix> m_projCB;
		std::shared_ptr<gk2::ConstantBuffer<XMFLOAT4>> m_lightPosCB;
		std::shared_ptr<gk2::ConstantBuffer<XMFLOAT4>> m_surfaceColorCB;
		std::shared_ptr<gk2::ConstantBuffer<XMFLOAT4>> m_cameraPosCB;

		std::shared_ptr<gk2::PhongEffect> m_phongEffect;
		std::shared_ptr<gk2::LightShadowEffect> m_lightShadowEffect;
		std::shared_ptr<gk2::ParticleSystem> m_particles;
		std::shared_ptr<ID3D11InputLayout> m_layout;

		std::shared_ptr<ID3D11RasterizerState> m_rsCullNone;
		std::shared_ptr<ID3D11BlendState> m_bsAlpha;
		std::shared_ptr<ID3D11DepthStencilState> m_dssNoWrite;

		void InitializeConstantBuffers();
		void InitializeCamera();
		void InitializeRenderStates();
		void CreateScene();
		void UpdateCamera();

		void DrawScene();
	};
}

#endif __GK2_ROOM_H_