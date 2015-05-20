#ifndef __GK2_ROOM_H_
#define __GK2_ROOM_H_

#include "gk2_applicationBase.h"
#include "gk2_meshLoader.h"
#include "gk2_camera.h"
#include "gk2_phongEffect.h"
#include "gk2_lightShadowEffect.h"
#include "gk2_constantBuffer.h"
#include "gk2_particles.h"
#include "gk2_textureEffect.h"

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
		static const XMFLOAT4 LIGHT_POS;

		gk2::Mesh m_walls[6];
		gk2::Mesh m_cylinder;
		gk2::Mesh m_sun;
		gk2::Mesh m_steelSheet;
		gk2::Mesh m_circle;
		gk2::Mesh m_puma;
		gk2::Mesh m_mirror;
		gk2::Mesh m_shadowVolumes[6];

		
		gk2::Mesh m_mesh1;
		gk2::Mesh m_mesh2;
		gk2::Mesh m_mesh3;
		gk2::Mesh m_mesh4;
		gk2::Mesh m_mesh5;
		gk2::Mesh m_mesh6;
		



		XMMATRIX m_projMtx;

		gk2::Camera m_camera;
		gk2::MeshLoader m_meshLoader;

		std::shared_ptr<gk2::CBMatrix> m_worldCB;
		std::shared_ptr<gk2::CBMatrix> m_viewCB;
		std::shared_ptr<gk2::CBMatrix> m_projCB;
		std::shared_ptr<gk2::CBMatrix> m_textureCB;
		std::shared_ptr<gk2::ConstantBuffer<XMFLOAT4>> m_lightPosCB;
		std::shared_ptr<gk2::ConstantBuffer<XMFLOAT4>> m_surfaceColorCB;
		std::shared_ptr<gk2::ConstantBuffer<XMFLOAT4>> m_cameraPosCB;
		XMMATRIX m_mirrorMtx;

		std::shared_ptr<gk2::PhongEffect> m_phongEffect;
		std::shared_ptr<gk2::TextureEffect> m_textureEffect;
		std::shared_ptr<gk2::LightShadowEffect> m_lightShadowEffect;
		std::shared_ptr<gk2::ParticleSystem> m_particles;
		std::shared_ptr<ID3D11InputLayout> m_layout;
		std::shared_ptr<ID3D11ShaderResourceView> m_wallTexture;
		std::shared_ptr<ID3D11ShaderResourceView> m_sunTexture;
		std::shared_ptr<ID3D11ShaderResourceView> m_steelSheetTexture;
		std::shared_ptr<ID3D11SamplerState> m_samplerWrap;
		std::shared_ptr<ID3D11SamplerState> m_samplerBorder;

		std::shared_ptr<ID3D11BlendState> m_bsAlpha;
		std::shared_ptr<ID3D11BlendState> m_bsAll;
		std::shared_ptr<ID3D11BlendState> m_bsNone;
		
		std::shared_ptr<ID3D11DepthStencilState> m_dssNoWrite;
		std::shared_ptr<ID3D11DepthStencilState> m_dssWrite;
		std::shared_ptr<ID3D11DepthStencilState> m_dssTest;
		std::shared_ptr<ID3D11DepthStencilState> m_dssIncr;
		std::shared_ptr<ID3D11DepthStencilState> m_dssDecr;
		std::shared_ptr<ID3D11DepthStencilState> m_dssKeepGreather;
		std::shared_ptr<ID3D11DepthStencilState> m_dssKeepEqual;
		std::shared_ptr<ID3D11DepthStencilState> m_dssNoStencil;
		std::shared_ptr<ID3D11DepthStencilState> m_dssStencil;

		std::shared_ptr<ID3D11RasterizerState> m_rsCounterClockwise;
		std::shared_ptr<ID3D11RasterizerState> m_rsCullNone;
		std::shared_ptr<ID3D11RasterizerState> m_rsCullBack;
		std::shared_ptr<ID3D11RasterizerState> m_rsCullFront;
		std::shared_ptr<ID3D11RasterizerState> m_rsDefault;

		void InitializeConstantBuffers();
		void InitializeCamera();
		void InitializeTextures();
		void InitializeRenderStates();
		void CreateScene();
		void UpdateCamera();
		void UpdateCamera(const XMMATRIX& view);

		
		void Room::CheckKeys(Camera& m_camera);
		void Room::UpdatePuma(float dt);
		void Room::InversedKinematic(XMVECTOR pos, XMVECTOR normal, float &a1, float &a2, float &a3, float &a4, float &a5);
		float angle;
		float steelWidth;
		

		void DrawScene();
		void DrawShadowScene();
		void DrawShadowVolumes();
		void DrawMirroredWorld();
		void DrawSteelSheet();
		void DrawPuma();
		void DrawWalls();
		void DrawCylinder();
		void DrawSun();
		void DrawCircle();
		void DrawMirror();
	};
}

#endif __GK2_ROOM_H_