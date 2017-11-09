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

	PUG_RESULT AllocateSRVDescriptors(
		uint32_t& out_heapIndex);
	PUG_RESULT AllocateRTVDescriptors(
		uint32_t& out_heapIndex);
	PUG_RESULT AllocateDSVDescriptors(
		uint32_t& out_heapIndex);
	PUG_RESULT AllocateUAVDescriptors(
		uint32_t& out_heapIndex);
	PUG_RESULT AllocateNSVUAVDescriptors(
		uint32_t& out_heapIndex);

	PUG_RESULT ReleaseSRVDescriptors(
		const uint32_t& inout_heapIndex);
	PUG_RESULT ReleaseRTVDescriptors(
		const uint32_t& inout_heapIndex);
	PUG_RESULT ReleaseDSVDescriptors(
		const uint32_t& inout_heapIndex);
	PUG_RESULT ReleaseUAVDescriptors(
		const uint32_t& inout_heapIndex);
	PUG_RESULT ReleaseNSVUAVDescriptors(
		const uint32_t& inout_heapIndex);

	/*
	PUG_RESULT AllocateTexture2D(
		ID3D12Resource* resource,
		const DXGI_FORMAT format,
		const vmath::Vector4& clearColor,
		const D3D12_RESOURCE_FLAGS& resourceFlags,
		const uint32_t width,
		const uint32_t height,
		const D3D12_RESOURCE_STATES& a_initialState,
		TextureHandle& out_textureHandle);

	PUG_RESULT ReleaseTexture2D(
		TextureHandle& inout_textureHandle);
	*/
}
}
}
///<todo>