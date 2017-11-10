#pragma once
#include "result_codes.h"
#include "typedef.h"

#include <cstdint>
#include <string>
#include <experimental/filesystem>

namespace pug{
namespace assets{
namespace graphics {

	PUG_RESULT InitShaderLibrarian();
	PUG_RESULT DestroyShaderLibrarian();

	void RegisterShaderReloadedCallback(pug::CallBack callback);
	bool GetShader(const std::experimental::filesystem::path& shaderFileName, uint8_t*& out_byteCode, uint64_t& out_byteCount);

}//pug::assets::graphics;
}//vpl::assets
}//vpl
