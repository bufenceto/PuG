#include "texture_reader.h"

#include "dds.h"
#include "importers/texture_converter.h"

#include "logger/logger.h"
#include "vmath/vmath.h"

#include <fstream>

namespace pug {
namespace assets {

	using std::experimental::filesystem::path;
	using vmath::Int2;
	using pug::assets::resource::DDS_HEADER;
	using pug::assets::resource::DDS_PIXELFORMAT;

	PUG_RESULT ReadSizeFromTexture(
		const path& a_absoluteCookedAssetPath,
		Int2& out_size)
	{
		//does this file exist and is it a file
		if (!exists(a_absoluteCookedAssetPath) || is_directory(a_absoluteCookedAssetPath))
		{
			log::Error("File does not exist!");
			return PUG_RESULT_INVALID_ARGUMENTS;
		}
		//is this a valid file
		if (a_absoluteCookedAssetPath.extension() != COOKED_TEXTURE_EXTENSION)
		{
			log::Error("File extension was invalid");
			return PUG_RESULT_INVALID_ARGUMENTS;
		}

		uint64_t bytesToRead = sizeof(DDS_HEADER) + sizeof(uint32_t);
		uint64_t fileSize = file_size(a_absoluteCookedAssetPath);
		// Need at least enough data to fill the header and magic number to be a valid DDS
		if (fileSize < bytesToRead)
		{
			log::Error("File is to small, can not contain a valid DDS file");
			return PUG_RESULT_INVALID_ARGUMENTS;
		}

		std::fstream ddsFile;
		ddsFile.open(a_absoluteCookedAssetPath, std::fstream::in | std::fstream::binary);

		int8_t* textureFileHeaderData = (int8_t*)_malloca(bytesToRead);
		uint32_t bytesRead = (uint32_t)ddsFile.read((char*)textureFileHeaderData, bytesToRead).gcount();
		if (bytesRead != bytesToRead)
		{
			//_aligned_free(textureFileHeaderData);
			return PUG_RESULT_FAILED_TO_READ_FILE;
		}

		// DDS files always start with the same magic number ("DDS ")
		uint32_t dwMagicNumber = *(const uint32_t*)(textureFileHeaderData);
		if (dwMagicNumber != DDS_MAGIC)
		{
			//memory leak!
			log::Error("Invalid dds file, magic number not present!\n");
			//_aligned_free(textureFileHeaderData);
			return PUG_RESULT_INVALID_ARGUMENTS;
		}

		DDS_HEADER* hdr = reinterpret_cast<DDS_HEADER*>(textureFileHeaderData + sizeof(uint32_t));

		// Verify header to validate DDS file
		if (hdr->size != sizeof(DDS_HEADER) ||
			hdr->ddspf.size != sizeof(DDS_PIXELFORMAT))
		{
			log::Error("Invalid dds file, invalid header size!\n");
			//_aligned_free(textureFileHeaderData);
			return PUG_RESULT_INVALID_ARGUMENTS;
		}

		// Check for DX10 extension
		//THIS SHOULD NEVER HAPPEN IN PUG!
		//if ((hdr->ddspf.flags & DDS_FOURCC) && (VPL_MAKEFOURCC('D', 'X', '1', '0') == hdr->ddspf.fourCC))
		//{
		//	log::Error("Invalid DDS file type!");
		//	//_aligned_free(textureFileHeaderData);
		//	return PUG_RESULT_INVALID_ARGUMENTS;//we do not support DXT10
		//}

		out_size = Int2(hdr->width, hdr->height);
		//PUG_RESULT res = PUG_RESULT_UNKNOWN;
		return PUG_RESULT_OK;
	}

}
}