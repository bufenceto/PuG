#pragma once
#include "result_codes.h"
#include "asset_types.h"

#include <experimental/filesystem>

namespace vmath {
	class Int2;
}

namespace pug {
namespace assets {



	class SHA1Hash;
	class AssetSettings;
	class Material;

	PUG_RESULT CreateAssetDataBase(
		const std::experimental::filesystem::path& a_dataBaseFilePath);
	PUG_RESULT DestroyAssetDataBase();

	void ImportAsset(
		const pug::assets::SHA1Hash& a_assetHash,
		const std::experimental::filesystem::path& a_relativeFilePath,
		AssetType a_assetType);

	void RemoveAsset(
		const pug::assets::SHA1Hash& a_assetHash,
		const std::experimental::filesystem::path& a_relativeFilePath);

	uint32_t IsItemInDatabase(
		const pug::assets::SHA1Hash& a_assetHash);

	uint32_t FindAssetSettingsForFile(
		const pug::assets::SHA1Hash& a_assetHash,
		AssetSettings& out_assetSettings);

	void SetAssetSettingsForFile(
		const pug::assets::SHA1Hash& a_assetHash,
		const pug::assets::AssetSettings& a_assetSettings);

	pug::assets::Material* FindMaterialForCookedMeshFile(
		const std::experimental::filesystem::path& a_relativeFilePath);

	vmath::Int2* FindSizeForCookedTextureFile(
		const std::experimental::filesystem::path& a_relativeFilePath);

}
}