#pragma once
#pragma once
#include <cstdint>

namespace pug {
namespace assets{
namespace resource {

#define BPE 16

#define DDS_FOURCC						0x00000004  // DDPF_FOURCC
#define DDS_HEADER_FLAGS_VOLUME         0x00800000  // DDSD_DEPTH

#define DDS_CUBEMAP_POSITIVEX 0x00000600 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEX
#define DDS_CUBEMAP_NEGATIVEX 0x00000a00 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEX
#define DDS_CUBEMAP_POSITIVEY 0x00001200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEY
#define DDS_CUBEMAP_NEGATIVEY 0x00002200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEY
#define DDS_CUBEMAP_POSITIVEZ 0x00004200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEZ
#define DDS_CUBEMAP_NEGATIVEZ 0x00008200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEZ

#define DDS_CUBEMAP_ALLFACES ( DDS_CUBEMAP_POSITIVEX | DDS_CUBEMAP_NEGATIVEX |\
								DDS_CUBEMAP_POSITIVEY | DDS_CUBEMAP_NEGATIVEY |\
								DDS_CUBEMAP_POSITIVEZ | DDS_CUBEMAP_NEGATIVEZ )

#define DDS_CUBEMAP 0x00000200 // DDSCAPS2_CUBEMAP

#ifndef VPL_MAKEFOURCC
#define VPL_MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
					((uint32_t)(uint8_t)(ch0) | ((uint32_t)(uint8_t)(ch1) << 8) |       \
					((uint32_t)(uint8_t)(ch2) << 16) | ((uint32_t)(uint8_t)(ch3) << 24 ))
#endif

	const uint32_t DDS_MAGIC = 0x20534444; // "DDS "

	struct DDS_PIXELFORMAT
	{
		uint32_t    size;
		uint32_t    flags;
		uint32_t    fourCC;
		uint32_t    RGBBitCount;
		uint32_t    RBitMask;
		uint32_t    GBitMask;
		uint32_t    BBitMask;
		uint32_t    ABitMask;
	};

	struct DDS_HEADER
	{
		uint32_t        size;
		uint32_t        flags;
		uint32_t        height;
		uint32_t        width;
		uint32_t        pitchOrLinearSize;
		uint32_t        depth; // only if DDS_HEADER_FLAGS_VOLUME is set in flags
		uint32_t        mipMapCount;
		uint32_t        reserved1[11];
		DDS_PIXELFORMAT ddspf;
		uint32_t        caps;
		uint32_t        caps2;
		uint32_t        caps3;
		uint32_t        caps4;
		uint32_t        reserved2;
	};

	static bool IsDataFormatBC3Unorm(const DDS_PIXELFORMAT& ddpf)
	{
		if (ddpf.flags & DDS_FOURCC)
		{
			if (VPL_MAKEFOURCC('D', 'X', 'T', '5') == ddpf.fourCC)
			{
				return true;
			}

			if (VPL_MAKEFOURCC('D', 'X', 'T', '4') == ddpf.fourCC)
			{
				return true;
			}
		}
		return false;
	}

}//pug::assets::resource
}//pug::assets
}//pug