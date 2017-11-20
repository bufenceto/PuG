#pragma once
#include "macro.h"
#include "resource/dx12_texture2D.h"

#include "vmath/vmath.h"

#include <cstdint>
#include <vector>
#include <d3d12.h>

namespace pug {
namespace assets{
namespace graphics {

	PUG_RESULT InitPersistentDescriptorHeap(
		ID3D12Device* a_device,
		uint32_t a_srvHeapCapacity,
		uint32_t a_dsvHeapCapacity,
		uint32_t a_rtvHeapCapacity,
		uint32_t a_uavHeapCapacity,
		uint32_t a_nsvUAVHeapCapacity);

	PUG_RESULT DestroyPersistentDescriptorHeap();

	uint32_t AllocatePersistentSRVDescriptors();
	uint32_t AllocatePersistentRTVDescriptors();
	uint32_t AllocatePersistentDSVDescriptors();
	uint32_t AllocatePersistentUAVDescriptors();
	uint32_t AllocatePersistentNSVUAVDescriptors();

	PUG_RESULT ReleasePersistentSRVDescriptors(
		const uint32_t& inout_heapIndex);
	PUG_RESULT ReleasePersistentRTVDescriptors(
		const uint32_t& inout_heapIndex);
	PUG_RESULT ReleasePersistentDSVDescriptors(
		const uint32_t& inout_heapIndex);
	PUG_RESULT ReleasePersistentUAVDescriptors(
		const uint32_t& inout_heapIndex);
	PUG_RESULT ReleasePersistentNSVUAVDescriptors(
		const uint32_t& inout_heapIndex);

	PUG_RESULT GetPersistentSRVDescriptors(
		const uint32_t& srvDescriptorIndex,
		D3D12_CPU_DESCRIPTOR_HANDLE* out_cpuHandle,
		D3D12_GPU_DESCRIPTOR_HANDLE* out_gpuHandle);
	PUG_RESULT GetPersistentRTVDescriptors(
		const uint32_t& rtvDescriptorIndex,
		D3D12_CPU_DESCRIPTOR_HANDLE* out_cpuHandle,
		D3D12_GPU_DESCRIPTOR_HANDLE* out_gpuHandle);
	PUG_RESULT GetPersistentDSVDescriptors(
		const uint32_t& dsvDescriptorIndex,
		D3D12_CPU_DESCRIPTOR_HANDLE* out_cpuHandle,
		D3D12_GPU_DESCRIPTOR_HANDLE* out_gpuHandle);
	PUG_RESULT GetPersistentUAVDescriptors(
		const uint32_t& uavDescriptorIndex,
		D3D12_CPU_DESCRIPTOR_HANDLE* out_cpuHandle,
		D3D12_GPU_DESCRIPTOR_HANDLE* out_gpuHandle);
	PUG_RESULT GetPersistentNSVUAVDescriptors(
		const uint32_t& nsvUAVDescriptorIndex,
		D3D12_CPU_DESCRIPTOR_HANDLE* out_cpuHandle,
		D3D12_GPU_DESCRIPTOR_HANDLE* out_gpuHandle);

	ID3D12DescriptorHeap* const GetPersistentSRVDescriptorHeap(
		const uint32_t& srvDescriptorIndex);
}
}
}
///<todo>