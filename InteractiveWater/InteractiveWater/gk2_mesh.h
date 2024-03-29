#ifndef __GK2_MESH_H_
#define __GK2_MESH_H_

#include <d3d11.h>
#include <xnamath.h>
#include <memory>

namespace gk2
{
	class Mesh
	{
	public:
		Mesh(std::shared_ptr<ID3D11Buffer> vb, unsigned int stride,
			 std::shared_ptr<ID3D11Buffer> ib, unsigned int indicesCount);
		Mesh();
		Mesh(const Mesh& right);

		const XMMATRIX& getWorldMatrix() const { return m_worldMtx; }
		void setWorldMatrix(const XMMATRIX& mtx) { m_worldMtx = mtx; }
		void Render(const std::shared_ptr<ID3D11DeviceContext>& context);
		void RenderLinear(const std::shared_ptr<ID3D11DeviceContext>& context);

		Mesh& operator =(const Mesh& right);

		static void* operator new(size_t size);
		static void operator delete(void* ptr);

	private:
		std::shared_ptr<ID3D11Buffer> m_vertexBuffer;
		std::shared_ptr<ID3D11Buffer> m_indexBuffer;
		unsigned int m_stride;
		unsigned int m_indicesCount;
		XMMATRIX m_worldMtx;
	};
}

#endif __GK2_MESH_H_
