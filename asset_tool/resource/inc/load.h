#pragma once
#include "result_codes.h"
#include "vertex.h"
#include "dds.h"
#include "transform.h"

#include <experimental/filesystem>

namespace pug {
namespace assets {
namespace resource {

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

	PUG_RESULT LoadMesh(
		const std::experimental::filesystem::path& path,
		pug::assets::graphics::Vertex**& out_vertices,
		uint32_t*& out_vertexCount,
		uint32_t**& out_indices,
		uint32_t*& out_indexCount,
		pug::assets::resource::RawMeshMaterial*& out_rawMaterials,
		uint32_t& out_meshCount);
	PUG_RESULT UnloadMesh(
		pug::assets::graphics::Vertex** vertices,
		uint32_t* vertexCounts,
		uint32_t** indices,
		uint32_t* indexCounts,
		RawMeshMaterial* rawMaterials,
		const uint32_t meshCount);

	PUG_RESULT LoadDDSTexture(
		const std::experimental::filesystem::path& path,
		uint8_t*& out_data,
		uint8_t*& out_textureData,
		uint64_t& out_dataSize,
		pug::assets::resource::DDS_HEADER& out_header);
	PUG_RESULT UnloadTexture(
		uint8_t* out_data);

}
}
}