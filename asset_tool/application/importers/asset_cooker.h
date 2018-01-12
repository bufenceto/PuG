#pragma once
#include "result_codes.h"
#include "asset_types.h"
#include "asset_settings.h"

#include <experimental/filesystem>

namespace pug {
namespace assets {

	PUG_RESULT InitAssetCooker(
		const std::experimental::filesystem::path& a_assetBasePath,
		const std::experimental::filesystem::path& a_absoluteAssetOutputPath);
	PUG_RESULT DestroyAssetCooker();
		
	PUG_RESULT SubmitCookJob(
		const std::experimental::filesystem::path& relativeAssetPath,
		const AssetSettings& a_assetSettings,
		const uint32_t a_forceCook = 0);
	
	PUG_RESULT RemoveCookJob(const std::experimental::filesystem::path& relativeAssetPath);

	bool IsJobActive(const std::experimental::filesystem::path& assetPath);
	bool IsJobQueued(const std::experimental::filesystem::path& assetPath);

	AssetType DetermineAssetType(const std::experimental::filesystem::path& assetPath);
	void GetCopyOfActiveJobList(std::vector<std::experimental::filesystem::path>& out_activeJobs);
	void GetCopyOfQueuedJobList(std::vector<std::experimental::filesystem::path>& out_activeJobs);
	
	const uint32_t GetCookedAssetPath(
		const std::experimental::filesystem::path& a_relativeRawAssetPath,
		std::experimental::filesystem::path& out_absoluteCookedAssetPath);
}
}