#pragma once
#include "dx12_resource.h"

#include "logger.h"

namespace pug {
namespace assets {
namespace graphics {

	class DX12IndexBuffer : public DX12Resource
	{
	public:
		DX12IndexBuffer()
			: DX12Resource()
			, m_view({})
			, m_indexCount(0)
		{}
		DX12IndexBuffer(
			ID3D12Resource* a_resource,
			const DXGI_FORMAT a_indexFormat,
			const uint32_t a_indexCount,
			const D3D12_RESOURCE_STATES a_initialState);
		~DX12IndexBuffer();

		const D3D12_INDEX_BUFFER_VIEW GetView() const
		{
			return m_view;
		}
		const size_t GetBufferSize() const
		{
			return m_view.SizeInBytes;
		}
		const size_t GetIndexSize() const;
		const uint32_t GetIndexCount() const
		{
			return m_indexCount;
		}

	private:
		D3D12_INDEX_BUFFER_VIEW m_view;
		uint32_t m_indexCount;
	};

}
}
}