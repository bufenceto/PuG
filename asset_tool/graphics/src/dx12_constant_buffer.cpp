#include "dx12_constant_buffer.h"

#include "dx12_helper.h"

#include "directx12/d3dx12.h"

namespace pug{
namespace assets{
namespace graphics
{
	ConstantBufferHeap::ConstantBufferHeap(ID3D12Device* a_device)
		: m_device(a_device),
		m_ptr(0)
	{
		if (!m_device)
		{
			return;
			}

		D3D12_HEAP_PROPERTIES properties = {};
		properties.CreationNodeMask = 0;
		properties.VisibleNodeMask = 0;
		properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		properties.Type = D3D12_HEAP_TYPE_UPLOAD;

		m_device->CreateCommittedResource(
			&properties,
			D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
			&CD3DX12_RESOURCE_DESC::Buffer(m_heapSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_heap)
		);

		CD3DX12_RANGE range(0, 0);
		if (FAILED(m_heap->Map(0, &range, reinterpret_cast<void**>(&m_dataBegin))))
		{
			log::Error("Error mapping constant buffer resource.");
		}
	}

	ConstantBufferHeap::~ConstantBufferHeap()
	{
		if (m_heap)
		{
			m_heap->Release();
		}
	}

	ConstantBufferHeap& ConstantBufferHeap::operator=(ConstantBufferHeap&& other)
	{
		memcpy(const_cast<ID3D12Device**>(&m_device), &other.m_device, sizeof(m_device));
		m_heap->Release();
		m_heap = other.m_heap;
		other.m_heap = nullptr;
		m_ptr = other.m_ptr;
		return *this;
	}

	void ConstantBufferHeap::Write(void* a_data, size_t a_size, size_t& out_offsetInHeap)
	{
		PUG_ASSERT(m_ptr % 256 == 0, "ptr unaligned!");

		a_size = DX12_CONSTANT_BUFFER_ELEMENT_SIZE(a_size);

		//If the data will overflow
		if (m_ptr + a_size > m_heapSize)
		{
			//Return to the beginning of the heap
			m_ptr = 0;
		}

		memcpy(m_dataBegin + m_ptr, a_data, a_size);
		out_offsetInHeap = m_ptr;
		m_ptr += a_size;
	}

}
}
}