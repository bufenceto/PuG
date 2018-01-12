#include "texture_converter.h"
#include "texconv/texconv.h"

#include "logger/logger.h"

#include <algorithm>
#include <Windows.h>
#include <experimental\filesystem>

using namespace pug;
using namespace pug::assets;

using std::experimental::filesystem::path;

char* GetConversionFormatStringFromSettings(const AssetSettings& a_assetSettings)
{
	switch (a_assetSettings.m_texSettings.m_compressionMethod)
	{
	case (TEXTURE_COMPRESSION_METHODS_BC1) :  return "BC1_UNORM";
	case (TEXTURE_COMPRESSION_METHODS_BC2) :  return "BC2_UNORM";
	case (TEXTURE_COMPRESSION_METHODS_BC3) :  return "BC3_UNORM";
	case (TEXTURE_COMPRESSION_METHODS_BC4) :  return "BC4_UNORM";
	case (TEXTURE_COMPRESSION_METHODS_BC5) :  return "BC5_UNORM";
	case (TEXTURE_COMPRESSION_METHODS_BC6) :  return "BC6_UNORM";
	case (TEXTURE_COMPRESSION_METHODS_BC7) :  return "BC7_UNORM";
	default:
		return "";
	}
}

void CreateCommandLineArgumentList(
	const std::string& a_asset,
	const std::string& a_outputDirectory,
	const AssetSettings& a_assetSettings,
	std::vector<char*>& out_argument_list)
{
												
}

TextureConverter::TextureConverter()
{

}

TextureConverter::~TextureConverter()
{

}

bool TextureConverter::IsExtensionSupported(
	const path& extension) const
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

PUG_RESULT TextureConverter::CookAsset(
	const path& a_asset,
	const path& a_outputDirectory,
	const AssetSettings a_assetSettings)
{
	if (a_assetSettings.m_type != AssetType_Texture)
	{
		log::Error("Incorrect asset type settings");
		return PUG_RESULT_INVALID_ARGUMENTS;
	}

	std::string assetPath = a_asset.string();
	std::string out = a_outputDirectory.parent_path().string();
	std::string format = GetConversionFormatStringFromSettings(a_assetSettings);// "BC3_UNORM";

	std::vector<char*> arguments_test;
	//CreateCommandLineArgumentList(
	//	assetPath,
	//	out,
	//	a_assetSettings,
	//	arguments_test);
	char buffer[8] = {};
	//format argument list
	{
		std::string format = GetConversionFormatStringFromSettings(a_assetSettings);

		//the path of the command line tool goes here normally
		arguments_test.push_back("EMPTY");

		//input path
		arguments_test.push_back(const_cast<char*>(assetPath.c_str()));

		//convert image dimensions to be a power of 2
		if (a_assetSettings.m_texSettings.m_makePowerOfTwo)
		{
			arguments_test.push_back("-pow2");
		}

		if (a_assetSettings.m_texSettings.m_compress)
		{
			//format indicator
			arguments_test.push_back("-f");
			//format string
			arguments_test.push_back(GetConversionFormatStringFromSettings(a_assetSettings));
		}

		//show timing
		arguments_test.push_back("-timing");

		//generate mips
		arguments_test.push_back("-m");
		//0 indicates all mip level should be generated
		
		_itoa_s(a_assetSettings.m_texSettings.m_numMipsToGenerate, buffer, sizeof(buffer), 10);
		arguments_test.push_back(buffer);

		//output indicator
		arguments_test.push_back("-o");
		//output directory
		arguments_test.push_back(const_cast<char*>(out.c_str()));

		//overwrite existing files
		arguments_test.push_back("-y");

		//do not print the logo
		arguments_test.push_back("-nologo");
	}

	int result = ConvertAndSaveTexture((int)arguments_test.size(), &arguments_test[0]);
	return result == 0 ? 1 : 0;
}

PUG_RESULT TextureConverter::UncookAsset(
	const path& absoluteCookedAssetPath)
{
	//delete output file
	//path absoluteAssetOutputPath = g_assetOutputPath / path(a_assetPath).replace_extension(a_assetEntry.m_extension);
	if (exists(absoluteCookedAssetPath))
	{
		if (DeleteFile(absoluteCookedAssetPath.string().c_str()) == 0)
		{
			log::Error("Failed to delete outputted asset file at: %s", absoluteCookedAssetPath.string().c_str());
		}
	}

	return PUG_RESULT_OK;
}