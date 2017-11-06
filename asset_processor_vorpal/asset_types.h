#pragma once
#include <cstdint>

namespace vpl
{
	enum class EAssetType : uint32_t
	{
		Unknown = 0,
		Mesh = 1,
		Texture = 2,
		Material = 3,
		Shader = 4,
		NumAssetTypes,
	};
}//vpl