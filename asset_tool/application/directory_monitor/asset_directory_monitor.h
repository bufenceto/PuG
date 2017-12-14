#pragma once
#include "result_codes.h"

#include <experimental/filesystem>

namespace pug {
namespace assets {

	enum PUG_FILE_ACTION
	{
		PUG_FILE_ACTION_UNKNOWN				= 0,
		PUG_FILE_ACTION_ADDED				= 1 << 0,
		PUG_FILE_ACTION_REMOVED				= 1 << 1,
		PUG_FILE_ACTION_MODIFIED			= 1 << 2,
		PUG_FILE_ACTION_RENAMED_OLD_NAME	= 1 << 3,
		PUG_FILE_ACTION_RENAMED_NEW_NAME	= 1 << 4,
	};

	struct DirectoryChange
	{
		PUG_FILE_ACTION fileAction;
		char fileName[256];
		uint32_t data;
	};

	typedef void(*ReportDirectoryChangeCallBack)(DirectoryChange a_dirChange);

	PUG_RESULT InitAssetDirectoryMonitor(
		const std::experimental::filesystem::path& a_absoluteAssetBasePath,
		ReportDirectoryChangeCallBack a_reportChangeCallBack);
	
	//PUG_RESULT RegisterFileForMonitoring(
	//	const std::experimental::filesystem::path& a_relativeAssetPath);
	//PUG_RESULT RemoveFileForMonitoring(
	//	const std::experimental::filesystem::path& a_relativeAssetPath);

	//PUG_RESULT UpdateAssetImporter();
	PUG_RESULT DestroyAssetDirectoryMonitor();
	
	//AssetType DetermineAssetType(const std::experimental::filesystem::path& assetPath);

}
}