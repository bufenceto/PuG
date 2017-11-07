#include <cstdio>
#include <memory>
#include <string>
#include <cassert>
#include <fstream>
#include <experimental\filesystem>

#include "Windows.h"

#include "logger.h"
#include "mesh_converter.h"
#include "texture_converter.h"

#include "../utility/hash.h"

#define MAX_PATH_SIZE 260
#define MAX_ASSET_ENTRIES 1024
#define ASSET_ENTRY_SIZE 32
#define LIBRARY_FILE_NAME "asset_library.mal"

using namespace std;
using namespace std::experimental::filesystem;
using namespace vpl;

using namespace pug;
using namespace pug::log;

const char* helpMessage =
"Please specify a valid absolute windows path to be parsed, all sub folders will be parsed aswell!\n"
;

AssetConverter* g_converters[] =
{
	new MeshConverter(),
	new TextureConverter(),
};

char g_assetEntries[ASSET_ENTRY_SIZE * MAX_ASSET_ENTRIES];
uint32_t g_numAssetEntries;
fstream g_assetLibraryFile;

void WriteAssetEntriesToFile()
{
	if (g_assetLibraryFile.is_open())
	{
		//write file header
		uint32_t assetEntrySize = ASSET_ENTRY_SIZE;
		if ((!g_assetLibraryFile.write((char*)&g_numAssetEntries, sizeof(g_numAssetEntries))))
		{
			Error("Failed to write asset count to library file!");
		}
		if ((!g_assetLibraryFile.write((char*)&assetEntrySize, sizeof(assetEntrySize))))
		{
			Error("Failed to write asset entry size to library file!");
		}
		//write file body
		if (!g_assetLibraryFile.write(g_assetEntries, ASSET_ENTRY_SIZE * g_numAssetEntries))
		{
			Error("Failed to write asset entries to library file!");
		}
	}
	else
	{
		Error("Failed to write to library file, file not open!");
	}
}

void FormatAndAddAssetEntry(
	const path& relativeAssetPath, 
	const EAssetType& type, 
	const char* extension)
{
	char assetEntry[ASSET_ENTRY_SIZE];
	utility::SHA1(relativeAssetPath.string(), assetEntry, SHA1_HASH_BYTES);//the hash function will write the result to the first 20 bytes after the passed ptr
	memcpy(assetEntry + SHA1_HASH_BYTES, &type, sizeof(uint32_t));
	memcpy(assetEntry + SHA1_HASH_BYTES + sizeof(uint32_t), extension, ASSET_ENTRY_SIZE - (SHA1_HASH_BYTES + sizeof(uint32_t)));

	//copy formated asset entry to asset buffer
	memcpy(g_assetEntries + (ASSET_ENTRY_SIZE * g_numAssetEntries), assetEntry, ASSET_ENTRY_SIZE);
	++g_numAssetEntries;
	assert(g_numAssetEntries <= MAX_ASSET_ENTRIES);
}

void CookAsset(const path& absoluteRawAssetPath, 
			   const path& outputDirectoryPath, 
			   const path& relativeAssetPath)
{
	AssetConverter* suitableConverter = nullptr;

	for (uint32_t i = 0; i < sizeof(g_converters) / sizeof(g_converters[0]); ++i)
	{
		path extension = absoluteRawAssetPath.extension();
		if (extension.string().length() > 8)
		{
			Error("Files that have an extension of more than 8 characters are not supported");
		}
		if (g_converters[i]->IsExtensionSupported(extension))
		{
			suitableConverter = g_converters[i];
			break;
		}
	}

	if (suitableConverter != nullptr)
	{//we found a converter that accepts these kind of files
		path outputPath = outputDirectoryPath / relativeAssetPath.parent_path();
		if (!exists(outputPath))
		{//create any non-existing folder structures
			create_directories(outputPath);
		}

		string rawAssetStem = absoluteRawAssetPath.stem().string();
		path absoluteCookedAssetPath = outputPath / (rawAssetStem + suitableConverter->GetExtension());
		if (exists(absoluteCookedAssetPath))
		{//existing output file found
			if (IS_NEWER(absoluteRawAssetPath, absoluteCookedAssetPath))
			{//needs re cooking
				Info("Updating asset from path: %s", absoluteRawAssetPath.string());
				if(!suitableConverter->CookAsset(absoluteRawAssetPath, absoluteCookedAssetPath))
				{//smth went wrong
					Error("Failed to update asset from path: %s\n  Check the log for details", absoluteRawAssetPath.string().c_str());
					return;
				}
			}
			else
			{
				Log("Skipped %s", absoluteRawAssetPath.string());
			}
		}
		else
		{//new file entry
			Info("Cooking new asset from path: %s", absoluteRawAssetPath.string());
			if (!suitableConverter->CookAsset(absoluteRawAssetPath, absoluteCookedAssetPath))
			{//smth went wrong
				Error("Failed to cook asset from path: %s\n  Check the log for details", absoluteRawAssetPath.string().c_str());
				return;
			}
		}

		FormatAndAddAssetEntry(relativeAssetPath, suitableConverter->GetAssetType(), suitableConverter->GetExtension());
	}
	else
	{
		Log("No suitable convert found for file: %s", absoluteRawAssetPath.string());
	}
}

int main(int argc, char* argv[])
{
	wchar_t executablePath[MAX_PATH_SIZE];
	GetModuleFileName(GetModuleHandleA(NULL), executablePath, MAX_PATH_SIZE);
	path exePath = executablePath;

	StartLog(exePath.parent_path().string());

	if (argc <= 1)
	{
		Error("No command specified. Use \'help\' for a detailed command list");
		return 1;
	}

	// help
	// Display some basic help (--help and -h work as well 
	// because people could try them intuitively)
	if (!strcmp(argv[1], "help") || !strcmp(argv[1], "--help") || !strcmp(argv[1], "-h") || !strcmp(argv[1], "-help"))
	{
		Info(helpMessage);
		return 1;
	}

	path inputFolderPath = canonical(argv[1]);
	path outputFolderPath = canonical(argv[1] + string("/../library/"));
	if (!exists(inputFolderPath))
	{
		Error("Specified input folder does not exist!");
		return 1;
	}
	if (!exists(outputFolderPath))
	{
		if (!create_directories(outputFolderPath))
		{
			Error("Failed to create output directory at %s!", outputFolderPath.string().c_str());
			return 1;
		}
	}

	path libraryFilePath = outputFolderPath / LIBRARY_FILE_NAME;
	g_numAssetEntries = 0;
	g_assetLibraryFile.open(libraryFilePath, fstream::out | fstream::binary | fstream::trunc);
	if (!g_assetLibraryFile.is_open())
	{
		Error("Failed to open library file!");
		return 1;
	}
	memset(g_assetEntries, 0, ASSET_ENTRY_SIZE * MAX_ASSET_ENTRIES);

	size_t len = inputFolderPath.string().length();//the length of the path of our asset root directory
	recursive_directory_iterator it = recursive_directory_iterator(inputFolderPath);
	for (recursive_directory_iterator end = recursive_directory_iterator(); it != end; ++it)
	{
		directory_entry dirEntry = *it;
		string absolutePath = dirEntry.path().string();
		if (!is_directory(dirEntry))
		{//this is a file!
			auto relBeg = absolutePath.begin() + (len + 1);//+1 for the additional seperator
			auto relEnd = absolutePath.end();
			string rel = string(relBeg, relEnd);
			CookAsset(dirEntry, outputFolderPath, rel);
		}
	}

	WriteAssetEntriesToFile();
	g_assetLibraryFile.close();
	EndLog();
	return 0;
}

