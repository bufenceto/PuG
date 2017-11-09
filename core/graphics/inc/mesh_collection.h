#pragma once
#include <d3d12.h>
#include <cstdint>
#include "result_codes.h"

namespace pug
{
namespace graphics
{
	class DX12Device;
	class VertexBufferHandle;
	class IndexBufferHandle;
	struct Vertex;

	class MeshCollection
	{
	public:
		MeshCollection(DX12Device* a_device);
		~MeshCollection();

		void operator=(MeshCollection& other);
		void operator=(MeshCollection&& other);

		PUG_RESULT CreateMesh(
			VertexBufferHandle& out_vbHandle,
			IndexBufferHandle& out_ibHandle,
			Vertex* vertexArray,
			uint32_t vertexCount,
			uint32_t* indexArray,
			uint32_t indexCount
		);

		D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView(uint32_t a_index);
		D3D12_INDEX_BUFFER_VIEW GetIndexBufferView(uint32_t a_index);

	private:
		DX12Device* const m_device;

		D3D12_VERTEX_BUFFER_VIEW vbViews[1024];
		D3D12_INDEX_BUFFER_VIEW ibViews[1024];
	};
}
}