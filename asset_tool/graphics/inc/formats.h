#pragma once
#include <cstdint>

namespace pug {
namespace assets {
namespace graphics {

	enum PUG_FORMAT
	{
		PUG_FORMAT_NONE = 0,

		//8 bit formats
		PUG_FORMAT_R8_UINT,
		PUG_FORMAT_RG8_UINT,
		PUG_FORMAT_RGB8_UINT,
		PUG_FORMAT_RGBA8_UINT,

		PUG_FORMAT_R8_UNORM,
		PUG_FORMAT_RG8_UNORM,
		PUG_FORMAT_RGB8_UNORM,
		PUG_FORMAT_RGBA8_UNORM,

		//16 bit formats
		PUG_FORMAT_R16_UINT,
		PUG_FORMAT_RG16_UINT,
		PUG_FORMAT_RGB16_UINT,
		PUG_FORMAT_RGBA16_UINT,

		PUG_FORMAT_R16_UNORM,
		PUG_FORMAT_RG16_UNORM,
		PUG_FORMAT_RGB16_UNORM,
		PUG_FORMAT_RGBA16_UNORM,

		//32 bit formats
		PUG_FORMAT_R32_UINT,

		//packed formats
		PUG_FORMAT_RGB10A2,
		PUG_FORMAT_RG11B10F,

		//float formats
		PUG_FORMAT_R16F,
		PUG_FORMAT_RGBA16F,
		PUG_FORMAT_R32F,
		PUG_FORMAT_RGBA32F,

		//depth formats
		PUG_FORMAT_D16,
		PUG_FORMAT_D24,
		PUG_FORMAT_D24S8,
		PUG_FORMAT_D32,
		PUG_FORMAT_D32F,

		//compressed format
		PUG_FORMAT_BC3_UNORM,

		PUG_FORMAT_COUNT,

		FORCE_ENUM_SIZE = 0xFFFFFFFF,
	};
	enum EResourceFlags : uint32_t
	{
		NONE = 0,
		RENDER_TARGET = 0x1,//this thing can be a render target
		DEPTH_STENCIL = 0x2,//this thing can be a depth stencil
		UNORDERED_ACCES = 0x4,//acces to this thing will be unordered
		DENY_SHADER_RESOURCE = 0x8,//this thing can NOT be a shader resource
	};


}
}
}