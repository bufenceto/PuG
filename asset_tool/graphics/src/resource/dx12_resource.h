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
		DX12Resource()
		{
			m_initialized = 0;
			m_currentState = D3D12_RESOURCE_STATE_COMMON;
			m_resource = nullptr;
		}
		virtual ~DX12Resource() = 0 
		{
			m_initialized = 0;
			m_currentState = D3D12_RESOURCE_STATE_COMMON;
			m_resource = nullptr;
		}

		D3D12_RESOURCE_BARRIER Transition(D3D12_RESOURCE_STATES a_newState);
		void SetName(const wchar_t* name)
		{
			m_resource->SetName(name);
		}

		ID3D12Resource* GetResource() const
		{
			return m_resource;
		}
		const D3D12_RESOURCE_STATES GetCurrentState() const
		{
			return m_currentState;
		}
		
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