#pragma once
#include "asset_converter.h"

#define COOKED_TEXTURE_EXTENSION ".dds"

namespace pug {
namespace assets{

	class TextureConverter : public AssetConverter
	{
	public:
		TextureConverter();
		~TextureConverter();

		bool IsExtensionSupported(
			const std::experimental::filesystem::path& extension) const override;
		PUG_RESULT CookAsset(
			const std::experimental::filesystem::path& asset,
			const std::experimental::filesystem::path& outputDirectory,
			const AssetSettings a_assetSettings) override;
		PUG_RESULT UncookAsset(
			const std::experimental::filesystem::path& absoluteCookedAssetPath) override;
		const char* GetExtension() const override { return COOKED_TEXTURE_EXTENSION; }
		const AssetType GetAssetType() const override { return AssetType_Texture; }

	private:

	};

}
}