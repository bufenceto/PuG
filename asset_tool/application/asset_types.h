#pragma once
#include <cstdint>

enum AssetType
{
	AssetType_Unknown = 0,
	AssetType_Mesh = 1,
	AssetType_Texture = 2,
	AssetType_Material = 3,
	AssetType_Shader = 4,
	AssetType_NumAssetTypes,

	AssetType_ForceSize = 0x7fffffff
};
static_assert(sizeof(AssetType) == 4, "Enum size is not 4!");