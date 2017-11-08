#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <cstdint>

namespace pug{
namespace assets {
namespace graphics {

	class DX12Resource
	{
	public:
		virtual ~DX12Resource() = 0 {}

		D3D12_RESOURCE_BARRIER Transition(D3D12_RESOURCE_STATES a_newState);
		uint32_t IsInitialized() const
		{
			return m_initialized;
		}

	protected:
		uint32_t m_initialized;
		D3D12_RESOURCE_STATES m_currentState;
		ID3D12Resource* m_resource ;
	};

}
}
}