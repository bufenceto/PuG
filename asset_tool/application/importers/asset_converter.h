#pragma once
#include "asset_types.h"
#include "result_codes.h"
#include "asset_settings.h"

#include <experimental/filesystem>

#define IS_NEWER(a,b) (last_write_time(a) > last_write_time(b))

namespace pug {
namespace assets{

	class AssetConverter
	{
	public:
		virtual ~AssetConverter() = 0 {}

		virtual bool IsExtensionSupported(
			const std::experimental::filesystem::path& a_extension) const = 0;
		virtual PUG_RESULT CookAsset(
			const std::experimental::filesystem::path& a_absoluteRawAssetInputPath,
			const std::experimental::filesystem::path& a_absoluteCookedAssetOutputPath,
			const AssetSettings a_assetSettings) = 0;
		virtual PUG_RESULT UncookAsset(
			const std::experimental::filesystem::path& absoluteCookedAssetPath) = 0;
		virtual const char* GetExtension() const = 0;
		virtual const AssetType GetAssetType() const = 0;
	};

}//pug::assets
}//pug