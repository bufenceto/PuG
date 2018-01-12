#pragma once
#include "result_codes.h"

#include <experimental/filesystem>

namespace vmath {
	class Int2;
}

namespace pug {
namespace assets {

	//read texture format?
	/*
		PUG_RESULT ReadMaterialFromMesh(
			const std::experimental::filesystem::path& a_absoluteCookedAssetPath,
			pug::assets::Material& out_material);
	*/

	PUG_RESULT ReadSizeFromTexture(
		const std::experimental::filesystem::path& a_absoluteCookedAssetPath,
		vmath::Int2& out_size);

	//read texture dimensions

}
}
