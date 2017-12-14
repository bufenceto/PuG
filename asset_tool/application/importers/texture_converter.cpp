#include "texture_converter.h"
#include "texconv/texconv.h"

#include <algorithm>

using namespace pug;
using namespace pug::assets;

TextureConverter::TextureConverter()
{

}

TextureConverter::~TextureConverter()
{

}

bool TextureConverter::IsExtensionSupported(
	const std::experimental::filesystem::path& extension) const
{
	std::string ext = extension.string();
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
	return (ext == ".tga" ||
			ext == ".dds" ||
			ext == ".jpg" ||
			ext == ".jpeg" ||
			ext == ".tiff" ||
			ext == ".gif" ||
			ext == ".png" ||
			ext == ".bmp");
}

uint32_t TextureConverter::CookAsset(
	const std::experimental::filesystem::path& a_asset,
	const std::experimental::filesystem::path& a_outputDirectory,
	const AssetSettings a_assetSettings)
{
	std::string path = a_asset.string();
	std::string out = a_outputDirectory.parent_path().string();
	std::string format = "BC3_UNORM";

	char* arguments[]
	{
		"EMPTY",													//the path of the command line tool goes here normally
		const_cast<char*>(path.c_str()),							//input path
		"-pow2",													//convert image dimensions to be a power of 2
		"-f",														//format indicator
		const_cast<char*>(format.c_str()),							//format string
		"-timing",													//show timing
		"-m",														//generate mips
		"0",														//0 indicates all mip level should be generated
		"-o",														//output indicator
		const_cast<char*>(out.c_str()),								//output directory
		"-y",														//overwrite existing files
		"-nologo"													//do not print to logo
	};
	int result = ConvertAndSaveTexture(sizeof(arguments) / sizeof(arguments[0]), arguments);
	return result == 0 ? 1 : 0;
}