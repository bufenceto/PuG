#pragma once
#include "asset_converter.h"
#include "macro.h"

#define COOKED_MESH_EXTENSION ".assbin"

namespace Assimp
{
	class Importer;
	class Exporter;
}


namespace pug {
namespace assets{

	class MeshConverter : public AssetConverter
	{
	public:
		MeshConverter();
		~MeshConverter();

		bool IsExtensionSupported(
			const std::experimental::filesystem::path& a_extension) const override;
		PUG_RESULT CookAsset(
			const std::experimental::filesystem::path& a_asset,
			const std::experimental::filesystem::path& a_outputDirectory,
			const AssetSettings a_assetSettings) override;
		PUG_RESULT UncookAsset(
			const std::experimental::filesystem::path& absoluteCookedAssetPath) override;
		const char* GetExtension() const override { return COOKED_MESH_EXTENSION; }
		const AssetType GetAssetType() const override { return AssetType_Mesh; }

	private:

	};

}//pug::assets
}//pug