#ifndef __GK2_ROOM_H_
#define __GK2_ROOM_H_

#include "gk2_applicationBase.h"
#include "gk2_meshLoader.h"
#include "gk2_camera.h"
#include "gk2_phongEffect.h"
#include "gk2_constantBuffer.h"
#include "gk2_textureEffect.h"
#include "gk2_colorTexEffect.h"

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
		std::vector<XMFLOAT3> deBoorsPoints;
		XMMATRIX baseDuckMatrix;
		float duckPositionParameter = 0.33f;
		float duckStepFactor = 0.1;

		gk2::Mesh m_walls[6];
		gk2::Mesh m_duck;
		gk2::Mesh m_water;

		XMMATRIX m_projMtx;

		gk2::Camera m_camera;
		gk2::MeshLoader m_meshLoader;

		std::shared_ptr<gk2::CBMatrix> m_worldCB;
		std::shared_ptr<gk2::CBMatrix> m_viewCB;
		std::shared_ptr<gk2::CBMatrix> m_projCB;
		std::shared_ptr<gk2::CBMatrix> m_textureCB;
		std::shared_ptr<gk2::CBMatrix> m_colorTextureCB;
		std::shared_ptr<gk2::ConstantBuffer<XMFLOAT4>> m_lightPosCB;
		std::shared_ptr<gk2::ConstantBuffer<XMFLOAT4>> m_surfaceColorCB;
		std::shared_ptr<gk2::ConstantBuffer<XMFLOAT4>> m_cameraPosCB;

		std::shared_ptr<gk2::PhongEffect> m_phongEffect;
		std::shared_ptr<gk2::TextureEffect> m_textureEffect;

		std::shared_ptr<ID3D11InputLayout> m_layout;
		std::shared_ptr<ID3D11ShaderResourceView> m_cubicMapTexture;
		std::shared_ptr<gk2::ColorTexEffect> m_colorTextureEffect;
		std::shared_ptr<ID3D11ShaderResourceView> m_duckTexture;
		std::shared_ptr<ID3D11ShaderResourceView> m_waterTexture;
		std::shared_ptr<ID3D11SamplerState> m_samplerWrap;

		std::shared_ptr<ID3D11BlendState> m_bsAlpha;
		
		std::shared_ptr<ID3D11DepthStencilState> m_dssWrite;
		std::shared_ptr<ID3D11DepthStencilState> m_dssNoWrite;
		std::shared_ptr<ID3D11DepthStencilState> m_dssTest;

		std::shared_ptr<ID3D11RasterizerState> m_rsCounterClockwise;
		std::shared_ptr<ID3D11RasterizerState> m_rsCullNone;

		void InitializeConstantBuffers();
		void InitializeCamera();
		void InitializeTextures();
		void InitializeRenderStates();
		void CreateScene();
		void Room::CheckKeys(Camera& m_camera);
		float angle;

		void UpdateCamera();
		void Room::UpdateDuck(float dt);

		void DrawScene();
		void DrawWalls();
		void DrawDuck();
		void DrawWater();

		double CalculateZeroSplineValue(std::vector<double> knots, int i, double t);
		double CalculateNSplineValue(std::vector<double> knots, int i, int n, double t);
		XMFLOAT3 GetDuckPosition(float t);
	};
}

#endif __GK2_ROOM_H_