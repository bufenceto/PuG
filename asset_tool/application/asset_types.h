#pragma once
#include <cstdint>

enum AssetType : uint32_t
{
	AssetType_Unknown = 0,
	AssetType_Mesh = 1,
	AssetType_Texture = 2,
	AssetType_Material = 3,
	AssetType_Shader = 4,
	AssetType_NumAssetTypes,
};