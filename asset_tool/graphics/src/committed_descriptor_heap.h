#pragma once
#include "texture_handle.h"
#include "macro.h"
#include "dx12_texture2D.h"

#include "vmath.h"

#include <cstdint>
#include <vector>
#include <d3d12.h>



namespace pug {
namespace assets{
namespace graphics {

	class DescriptorPage;
	class DX12Texture2D;

	PUG_RESULT InitCommittedDescriptorHeap(
		ID3D12Device* a_device,
		uint32_t a_srvHeapCapacity,
		uint32_t a_dsvHeapCapacity,
		uint32_t a_rtvHeapCapacity,
		uint32_t a_uavHeapCapacity,
		uint32_t a_nsvUAVHeapCapacity);

	PUG_RESULT DestroyCommittedDescriptorHeap();

	PUG_RESULT AllocateComittedSRVDescriptors(
		uint32_t& out_heapIndex);
	PUG_RESULT AllocateComittedRTVDescriptors(
		uint32_t& out_heapIndex);
	PUG_RESULT AllocateComittedDSVDescriptors(
		uint32_t& out_heapIndex);
	PUG_RESULT AllocateComittedUAVDescriptors(
		uint32_t& out_heapIndex);
	PUG_RESULT AllocateComittedNSVUAVDescriptors(
		uint32_t& out_heapIndex);

	PUG_RESULT ReleaseComittedSRVDescriptors(
		const uint32_t& inout_heapIndex);
	PUG_RESULT ReleaseComittedRTVDescriptors(
		const uint32_t& inout_heapIndex);
	PUG_RESULT ReleaseComittedDSVDescriptors(
		const uint32_t& inout_heapIndex);
	PUG_RESULT ReleaseComittedUAVDescriptors(
		const uint32_t& inout_heapIndex);
	PUG_RESULT ReleaseComittedNSVUAVDescriptors(
		const uint32_t& inout_heapIndex);

	PUG_RESULT GetCommittedSRVDescriptors(
		const uint32_t& rtvDescriptorIndex,
		D3D12_CPU_DESCRIPTOR_HANDLE& out_cpuHandle,
		D3D12_GPU_DESCRIPTOR_HANDLE& out_gpuHandle);
	PUG_RESULT GetCommittedRTVDescriptors(
		const uint32_t& rtvDescriptorIndex,
		D3D12_CPU_DESCRIPTOR_HANDLE& out_cpuHandle,
		D3D12_GPU_DESCRIPTOR_HANDLE& out_gpuHandle);
}
}
}
///<todo>