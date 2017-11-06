#pragma once

#include <experimental\filesystem>
#include "asset_types.h"
#include "result_codes.h"

#define IS_NEWER(a,b) (last_write_time(a) > last_write_time(b))

namespace vpl {

	class AssetConverter
	{
	public:
		virtual bool IsExtensionSupported(
			const std::experimental::filesystem::path& extension) const = 0;
		virtual RESULT CookAsset(
			const std::experimental::filesystem::path& absoluteRawAssetInputPath,
			const std::experimental::filesystem::path& absoluteCookedAssetOutputPath) const = 0;
		virtual const char* GetExtension() const = 0;
		virtual const EAssetType GetAssetType() const = 0;
	};

}