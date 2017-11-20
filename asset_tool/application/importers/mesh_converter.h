#pragma once
#include "asset_converter.h"

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
			const std::experimental::filesystem::path& extension) const override;
		uint32_t CookAsset(
			const std::experimental::filesystem::path& asset, 
			const std::experimental::filesystem::path& outputDirectory) const override;
		const char* GetExtension() const override { return COOKED_MESH_EXTENSION; }
		const AssetType GetAssetType() const override { return AssetType_Mesh; }

	private:
		Assimp::Importer* m_importer;
		Assimp::Exporter* m_exporter;
	};

}//pug::assets
}//pug