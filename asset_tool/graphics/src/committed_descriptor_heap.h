#pragma once
#include "texture_handle.h"
#include "macro.h"
#include "dx12_texture2D.h"

#include "vmath.h"

#include <cstdint>
#include <vector>
#include <d3d12.h>

#define MAX_TEXTURES 1024

namespace pug {
namespace assets{
namespace graphics {

	class DescriptorPage;
	class DX12Texture2D;

	PUG_RESULT InitCommittedResourceHeap(
		ID3D12Device* a_device,
		uint32_t a_srvHeapCapacity,
		uint32_t a_dsvHeapCapacity,
		uint32_t a_rtvHeapCapacity,
		uint32_t a_uavHeapCapacity,
		uint32_t a_nsvUAVHeapCapacity);

	PUG_RESULT DestroyCommittedResourceHeap();

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

}
}
}
///<todo>