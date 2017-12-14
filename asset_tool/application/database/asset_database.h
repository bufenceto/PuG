#pragma once
#include "result_codes.h"
#include "sha1.h"
#include "asset_types.h"
#include "importers/asset_settings.h"
#include "loaders/material.h"

#include <experimental/filesystem>

namespace pug {
namespace assets {

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
		const AssetSettings& a_assetSettings);

	Material* FindMaterialForCookedMeshFile(
		const std::experimental::filesystem::path& a_relativeFilePath);
}
}