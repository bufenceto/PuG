#pragma once
#include "dx12_resource.h"

#include "logger.h"

namespace pug {
namespace assets {
namespace graphics {

	class DX12VertexBuffer : public DX12Resource
	{
	public:
		DX12VertexBuffer() 
			: DX12Resource()
			, m_view({})
			, m_vertexCount(0)
		{}
		DX12VertexBuffer(
			ID3D12Resource* a_resource,
			const size_t a_vertexStride_bytes,
			const uint32_t a_vertexCount,
			const D3D12_RESOURCE_STATES a_initialState);
		~DX12VertexBuffer();

		const D3D12_VERTEX_BUFFER_VIEW GetView() const
		{
			return m_view;
		}
		const size_t GetBufferSize() const
		{
			return m_view.SizeInBytes;
		}
		const size_t GetVertexSize() const
		{
			return m_view.StrideInBytes;
		}
		const uint32_t GetVertexCount() const
		{
			return m_vertexCount;
		}

	private:
		D3D12_VERTEX_BUFFER_VIEW m_view;
		uint32_t m_vertexCount;
	};

}
}
}