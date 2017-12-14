#pragma once
#include "material.h"

#include <experimental/filesystem>

namespace pug{
namespace assets{

	const uint32_t ReadMaterialFromMesh(
		const std::experimental::filesystem::path& a_absoluteCookedAssetPath,
		pug::assets::Material& out_material);

}
}
