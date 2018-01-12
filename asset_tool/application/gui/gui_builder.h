#pragma once
#include "sha1.h"
#include "asset_types.h"
#include "importers/asset_settings.h"

#include <cstdint>
#include <experimental/filesystem>
#include <vector>
#include <string>

namespace pug {
namespace assets {

#define ITEM_DETAILS_GUI_RESULT uint32_t

#define ITEM_DETAILS_GUI_RESULT_NOTHING 0
#define ITEM_DETAILS_GUI_RESULT_IMPORT_ITEM 1
#define ITEM_DETAILS_GUI_RESULT_REMOVE_ITEM 2
#define ITEM_DETAILS_GUI_RESULT_OPTION_CLICKED 3

	uint32_t BuildDirectoryStructureWindow(
		const std::experimental::filesystem::path& a_assetBasePath);

	ITEM_DETAILS_GUI_RESULT BuildAssetDetailWindow(
		const std::experimental::filesystem::path& a_relativeAssetPath,
		const AssetType a_assetType);

	void BuildActiveJobWindow(
		const std::vector<std::experimental::filesystem::path>& a_activeJobPaths);
	void BuildQueuedJobWindow(
		const std::vector<std::experimental::filesystem::path>& a_queuedJobPaths);

	const std::string GetCurrentSelectedItemString();

}
}