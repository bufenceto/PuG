#pragma once
#include <experimental/filesystem>


static std::experimental::filesystem::path MakePathRelativeToAssetBasePath(
	std::experimental::filesystem::path a_assetPath,
	std::experimental::filesystem::path a_basePath)
{
	if (!a_assetPath.is_absolute())
	{
		a_assetPath = std::experimental::filesystem::canonical(a_assetPath);
	}

	std::string absoluteAssetPath = a_assetPath.string();
	return std::string(absoluteAssetPath.begin() + a_basePath.string().length() + 1, absoluteAssetPath.end());
}