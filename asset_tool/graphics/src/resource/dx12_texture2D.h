#pragma once
#include "dx12_resource.h"

#include "logger.h"

namespace pug {
namespace assets{
namespace graphics {

	class DX12Texture2D : public DX12Resource
	{
	public:
		DX12Texture2D()
			: DX12Resource()
			, m_srvHeapIndex(0)
			, m_dsvHeapIndex(0)
			, m_rtvHeapIndex(0)
			, m_uavHeapIndex(0)
			, m_nsvUAVHeapIndex(0)
			, m_width(0)
			, m_height(0)
			, m_format(DXGI_FORMAT_UNKNOWN)
		{}

		DX12Texture2D(
			ID3D12Resource* a_resource,
			uint32_t a_srvHeapIndex,
			uint32_t a_dsvHeapIndex,
			uint32_t a_rtvHeapIndex,
			uint32_t a_uavHeapIndex,
			uint32_t a_nsvUAVHeapIndex,
			uint32_t a_width,
			uint32_t a_height,
			DXGI_FORMAT a_format,
			D3D12_RESOURCE_STATES a_initialState);

		~DX12Texture2D();

		const uint32_t& GetSRVHeapIndex() const
		{
			return m_srvHeapIndex;
		}
		const uint32_t& GetDSVHeapIndex() const
		{
			return m_dsvHeapIndex;
		}
		const uint32_t& GetRTVHeapIndex() const
		{
			return m_rtvHeapIndex;
		}
		const uint32_t& GetUAVHeapIndex() const
		{
			return m_uavHeapIndex;
		}
		const uint32_t& GetNSVUAVHeapIndex() const
		{
			return m_nsvUAVHeapIndex;
		}

	private:
		uint32_t m_srvHeapIndex;
		uint32_t m_dsvHeapIndex;
		uint32_t m_rtvHeapIndex;
		uint32_t m_uavHeapIndex;
		uint32_t m_nsvUAVHeapIndex;

		uint32_t m_width;
		uint32_t m_height;
		DXGI_FORMAT m_format;
	};

}
}
}
