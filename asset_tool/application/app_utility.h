#pragma once
#include <experimental/filesystem>

std::experimental::filesystem::path GetAssetBasePath();
std::experimental::filesystem::path GetAssetOutputPath();

static void MakePathRelativeToAssetBasePath(
	std::experimental::filesystem::path a_assetPath,
	std::experimental::filesystem::path a_basePath,
	std::experimental::filesystem::path& out_result)
{
	if (!a_assetPath.is_absolute())
	{
		a_assetPath = std::experimental::filesystem::canonical(a_assetPath);
	}
	
	std::string absoluteAssetPath = a_assetPath.string();
	out_result = std::string(absoluteAssetPath.begin() + a_basePath.string().length() + 1, absoluteAssetPath.end());
}