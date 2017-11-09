#pragma once
#include "vorpal_result_codes.h"
#include "vertex.h"
#include "dds.h"
#include "transform.h"

#include <experimental/filesystem>

namespace vpl {
namespace resource{

	//struct so we can return the data needed from the LoadMesh function
	//and parse it safely in the asset librarian
	struct RawMeshMaterial
	{
		std::experimental::filesystem::path diffuseTexturePath;//albedo
		std::experimental::filesystem::path specularTexturePath;//roughness
		std::experimental::filesystem::path normalTexturePath;//normal or bump
		std::experimental::filesystem::path emissiveTexturePath;//emissive

		vmath::Vector4 ambient;
		vmath::Vector4 diffuse;
		vmath::Vector4 specular;
		vmath::Vector4 emissive;
	};

	struct SceneNode
	{
		vmath::Matrix4 transform;//offset
		uint32_t materialIndex;
		uint32_t meshIndex;
	};

	RESULT LoadMesh(
		const std::experimental::filesystem::path& path,
		vpl::graphics::Vertex**& out_vertices,
		uint32_t*& out_vertexCount,
		uint32_t**& out_indices,
		uint32_t*& out_indexCount,
		vpl::resource::RawMeshMaterial*& out_rawMaterials,
		uint32_t& out_meshCount);
	RESULT UnloadMesh(
		vpl::graphics::Vertex** vertices,
		uint32_t* vertexCounts,
		uint32_t** indices,
		uint32_t* indexCounts,
		RawMeshMaterial* rawMaterials,
		const uint32_t meshCount);
	
	RESULT LoadDDSTexture(
		const std::experimental::filesystem::path& path,
		uint8_t*& out_data,
		uint8_t*& out_textureData,
		uint64_t& out_dataSize,
		vpl::resource::DDS_HEADER& out_header);
	RESULT UnloadTexture(
		uint8_t* out_data);
}
}