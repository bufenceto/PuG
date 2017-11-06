#pragma once
#include "asset_converter.h"

#define COOKED_TEXTURE_EXTENSION ".dds"

namespace vpl {

	class TextureConverter : public AssetConverter
	{
	public:
		TextureConverter();
		~TextureConverter();

		bool IsExtensionSupported(
			const std::experimental::filesystem::path& extension) const override;
		uint32_t CookAsset(
			const std::experimental::filesystem::path& asset,
			const std::experimental::filesystem::path& outputDirectory) const override;
		const char* GetExtension() const override { return COOKED_TEXTURE_EXTENSION; }
		const EAssetType GetAssetType() const override { return EAssetType::Texture; }

	private:

	};

}