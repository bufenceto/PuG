#pragma once
#include "material.h"
#include "result_codes.h"

#include <experimental/filesystem>
#include <vector>

namespace pug{
namespace assets{

	PUG_RESULT ReadMaterialFromMesh(
		const std::experimental::filesystem::path& a_absoluteCookedAssetPath,
		pug::assets::Material& out_material);
	/*
	PUG_RESULT ReadDependenciesFromMesh(
		const std::experimental::filesystem::path& a_absoluteCookedAssetPath,
		std::vector<pug::assets::Material>& out_materials);
	*/
}
}
