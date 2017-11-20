#pragma once
#include "vmath\vmath.h"
#include <d3d12.h>
#include "macro.h"

namespace pug{
namespace assets{
namespace graphics{

	struct SceneCB
	{
		vmath::Matrix4 view;
		vmath::Matrix4 projection;
		float padding[32];
	};
	static_assert(sizeof(SceneCB) % 256 == 0, "Size of SceneCB is invalid.");

	struct ObjectCB
	{
		vmath::Matrix4 transform;
		float padding[48];
	};
	static_assert(sizeof(ObjectCB) % 256 == 0, "Size of SceneCB is invalid.");

	//REMOVE-!-!-!-!
	struct TestCB
	{
		vmath::Vector4 color;
		float padding[60];
	};
	static_assert(sizeof(TestCB) % 256 == 0, "Size of TestCB is invalid.");

	class ConstantBufferHeap
	{
	public:
		ConstantBufferHeap(ID3D12Device* device);
		~ConstantBufferHeap();

		ConstantBufferHeap& operator=(ConstantBufferHeap&& other);

		D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() { return m_heap->GetGPUVirtualAddress(); }
		void Write(void* a_data, size_t a_size, size_t& out_offsetInHeap);

	private:
		ID3D12Device* const m_device;
		ID3D12Resource* m_heap;
		const size_t m_heapSize = MB(4);
		size_t m_ptr;

		uint8_t* m_dataBegin;
	};

}
}
}