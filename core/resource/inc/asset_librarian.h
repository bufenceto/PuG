#pragma once
#include "vorpal_result_codes.h"
#include "vorpal_typedef.h"

#include <experimental\filesystem>

namespace vpl{

namespace graphics{
	struct Mesh;
	struct Material;
	struct Transform;
}//vpl::graphics
	
namespace resource{

	struct Asset
	{
		char guid[20];
		uint32_t id;
		uint32_t type;
		float padding;
	};//32 bytes, hmmm alignment *drool*


	RESULT InitAssetLibrarian();
	RESULT ClearAssetLibrarian();

	RESULT LoadAsset(
		std::experimental::filesystem::path relativeAssetPath);//pass by copy
	RESULT UnloadAsset(
		const std::experimental::filesystem::path& relativeAssetPath);

	RESULT GetMeshAsset(
		const std::experimental::filesystem::path& relativeAssetPath,
		vpl::graphics::Mesh* out_meshes,
		vpl::graphics::Material* out_materials,
		vpl::graphics::Transform* out_meshOffsets,
		const uint32_t maxMeshCount,
		uint32_t& out_meshCount);
	RESULT GetMeshAsset(
		const std::experimental::filesystem::path& relativeAssetPath, 
		vpl::graphics::Mesh& out_result,
		vpl::graphics::Material& out_material);
	RESULT ReleaseMeshAsset(
		vpl::graphics::Mesh& meshAsset);

	RESULT GetTextureAsset(
		const std::experimental::filesystem::path& relativeAssetPath,
		vpl::resource::TextureAssetID& out_result);
	RESULT DereferenceTextureAssetID(
		const vpl::resource::TextureAssetID textureAssetID,
		vpl::graphics::TextureID& out_result);
	RESULT ReleaseTextureAsset(
		vpl::resource::TextureAssetID& textureAsset);

}//vpl::resource
}//vpl