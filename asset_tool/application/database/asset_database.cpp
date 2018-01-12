#include "asset_database.h"

#include "sha1.h"
#include "importers/asset_settings.h"
#include "importers/asset_cooker.h"
#include "loaders/mesh_reader.h"
#include "loaders/texture_reader.h"
#include "loaders/material.h"

#include "macro.h"

#include "logger/logger.h"
#include "vmath/vmath.h"

#include <vector>

using namespace pug;
using namespace pug::assets;
using namespace pug::log;
using namespace std;
using namespace std::experimental::filesystem;

using vmath::Int2;

static path g_assetDataBaseFile;

static vector<SHA1Hash> g_importedAssets;
static vector<AssetSettings> g_importedAssetSettings;
static vector<Material> g_importedMeshMaterials;
static vector<Int2> g_importedTextureSizes;

static int32_t FindAssetInImportedAssetList(const SHA1Hash& a_assetHash)
{
	int32_t assetIndex = -1;
	for (int32_t i = 0; i < (int32_t)g_importedAssets.size(); ++i)
	{
		if (g_importedAssets[i] == a_assetHash)
		{
			assetIndex = i;
			break;
		}
	}
	return assetIndex;
}

static bool ImportAssetSettingsFromFile(const path& a_assetDataBaseFile)
{
	if (!exists(a_assetDataBaseFile))
	{
		//log::Error("asset database file not found at %s", a_assetDataBaseFile);
		return false;
	}

	constexpr size_t elementSize = sizeof(SHA1Hash) + sizeof(AssetSettings);
	size_t fileSize = file_size(a_assetDataBaseFile);
	size_t numElements = fileSize / elementSize;

	FILE* assetSettingsFile = nullptr;
	fopen_s(&assetSettingsFile, a_assetDataBaseFile.string().c_str(), "rb");
	if (assetSettingsFile == nullptr)
	{
		log::Error("Failed to open asset database file for reading");
		return false;
	}

	int8_t* fileBuffer = (int8_t*)_malloca(fileSize);
	if (fileSize > 0)
	{
		if (fread_s(fileBuffer, fileSize, fileSize, 1, assetSettingsFile) != 1)
		{
			log::Error("Failed to read all elements from asset database file");
			return false;
		}

		for (size_t i = 0; i < numElements; ++i)
		{
			SHA1Hash hash = {};
			AssetSettings settings = {};

			size_t baseOffset = i * elementSize;
			memcpy(&hash, fileBuffer + baseOffset, sizeof(SHA1Hash));
			memcpy(&settings, fileBuffer + baseOffset + sizeof(SHA1Hash), sizeof(AssetSettings));

			g_importedAssets.push_back(hash);
			g_importedAssetSettings.push_back(settings);
			g_importedMeshMaterials.push_back(Material());
			g_importedTextureSizes.push_back(Int2());
		}
	}

	if (fclose(assetSettingsFile) == EOF)
	{
		log::Error("Failed to close asset database file");
		return false;
	}

	//brute force for file monitors, 
	//recursive_directory_iterator

	_freea(fileBuffer);
	return true;
}

static bool FlushAssetSettingsToFile(const path& a_assetDataBaseFile)
{
	PUG_ASSERT(g_importedAssets.size() == g_importedAssetSettings.size(), "lists out of sync");

	size_t hashSize_bytes = sizeof(SHA1Hash);
	size_t settingsSize_bytes = sizeof(AssetSettings);
	size_t numAssets = g_importedAssets.size();

	size_t elementSize = hashSize_bytes + settingsSize_bytes;
	size_t bufferSize = elementSize * numAssets;

	int8_t* dataBuffer = (int8_t*)_malloca(bufferSize);
	for (uint32_t i = 0; i < numAssets; ++i)
	{
		size_t baseOffset = elementSize * i;
		memcpy(dataBuffer + baseOffset, &(g_importedAssets[i]), sizeof(SHA1Hash));
		memcpy(dataBuffer + baseOffset + sizeof(SHA1Hash), &(g_importedAssetSettings[i]), sizeof(AssetSettings));
	}

	FILE* assetSettingsFile = nullptr;
	if (exists(a_assetDataBaseFile))
	{
		fopen_s(&assetSettingsFile, a_assetDataBaseFile.string().c_str(), "wb");
	}
	else
	{
		fopen_s(&assetSettingsFile, a_assetDataBaseFile.string().c_str(), "ab");
		log::Message("Created asset database file");
	}

	if (assetSettingsFile != nullptr)
	{
		size_t elementsWritten = fwrite(dataBuffer, elementSize, numAssets, assetSettingsFile);
		if (elementsWritten != numAssets)
		{
			log::Error("Failed to write all assets to database file!");
			return false;
		}
	}
	else
	{
		log::Error("Failed to write asset database to file buffer");
		return false;
	}
	if (fclose(assetSettingsFile) == EOF)
	{
		log::Error("Failed to close asset database file and apply changes");
		return false;
	}

	_freea(dataBuffer);
	return true;
}

PUG_RESULT pug::assets::CreateAssetDataBase(
	const path& a_dataBaseFilePath)
{
	g_assetDataBaseFile = a_dataBaseFilePath;
	if (!ImportAssetSettingsFromFile(a_dataBaseFilePath))
	{
		PUG_RESULT_FAILED_TO_READ_FILE;
	}
	return PUG_RESULT_OK;
}

PUG_RESULT pug::assets::DestroyAssetDataBase()
{
	if (!FlushAssetSettingsToFile(g_assetDataBaseFile))
	{
		return PUG_RESULT_FAILED_TO_WRITE_FILE;
	}
	return PUG_RESULT_OK;
}

void pug::assets::ImportAsset(
	const SHA1Hash& a_assetHash,
	const path& a_relativeFilePath,
	AssetType a_assetType)
{
	PUG_ASSERT(a_assetHash == SHA1Hash(a_relativeFilePath.string().c_str()), "Path hash does not evaluate to passed Hash");

	AssetSettings settings = AssetSettings(a_assetType);
	g_importedAssets.push_back(a_assetHash);
	g_importedAssetSettings.push_back(settings);
	g_importedMeshMaterials.push_back(Material());//push empty material
	g_importedTextureSizes.push_back(Int2());//push empty size
	//flush new contents to file
	SubmitCookJob(a_relativeFilePath, settings);
	FlushAssetSettingsToFile(g_assetDataBaseFile);
}

void pug::assets::RemoveAsset(
	const SHA1Hash& a_assetHash,
	const path& a_relativeFilePath)
{
	int32_t assetIndex = FindAssetInImportedAssetList(a_assetHash);
	if (assetIndex != -1)
	{
		std::swap(g_importedAssets[assetIndex], g_importedAssets.back());
		g_importedAssets.pop_back();

		std::swap(g_importedAssetSettings[assetIndex], g_importedAssetSettings.back());
		g_importedAssetSettings.pop_back();

		std::swap(g_importedMeshMaterials[assetIndex], g_importedMeshMaterials.back());
		g_importedMeshMaterials.pop_back();
	}

	RemoveCookJob(a_relativeFilePath);
	FlushAssetSettingsToFile(g_assetDataBaseFile);
}

uint32_t pug::assets::IsItemInDatabase(
	const SHA1Hash& a_assetHash)
{
	return FindAssetInImportedAssetList(a_assetHash) != -1;
}

uint32_t pug::assets::FindAssetSettingsForFile(
	const SHA1Hash& a_assetHash,
	AssetSettings& out_assetSettings)
{
	uint32_t assetIndex = FindAssetInImportedAssetList(a_assetHash);
	if (assetIndex != -1)
	{
		out_assetSettings = g_importedAssetSettings[assetIndex];
	}
	return (assetIndex != -1);
}

void pug::assets::SetAssetSettingsForFile(
	const pug::assets::SHA1Hash& a_assetHash,
	const AssetSettings& a_assetSettings)
{
	uint32_t assetIndex = FindAssetInImportedAssetList(a_assetHash);
	if (assetIndex != -1)
	{
		g_importedAssetSettings[assetIndex] = a_assetSettings;
	}
}

Material* pug::assets::FindMaterialForCookedMeshFile(
	const path& a_relativeFilePath)
{
	Material* res = nullptr;
	SHA1Hash assetHash = a_relativeFilePath.string().c_str();
	int32_t assetIndex = FindAssetInImportedAssetList(assetHash);
	if (assetIndex != -1)
	{//this asset is in the imported asset list, it could still be cooking
		if (g_importedMeshMaterials[assetIndex].IsInitialized() == 0)
		{
			path cookedAssetPath = path();
			if (GetCookedAssetPath(a_relativeFilePath, cookedAssetPath))
			{
				if(ReadMaterialFromMesh(cookedAssetPath, g_importedMeshMaterials[assetIndex]) == PUG_RESULT_OK)
				{
					res = &g_importedMeshMaterials[assetIndex];
				}
			}
		}
		else
		{
			res = &g_importedMeshMaterials[assetIndex];
		}
	}
	return res;
}


vmath::Int2* pug::assets::FindSizeForCookedTextureFile(
	const std::experimental::filesystem::path& a_relativeFilePath)
{
	Int2* res = nullptr;
	SHA1Hash assetHash = a_relativeFilePath.string().c_str();
	int32_t assetIndex = FindAssetInImportedAssetList(assetHash);
	if (assetIndex != -1)
	{//this asset is in the imported asset list, it could still be cooking
		if (g_importedTextureSizes[assetIndex] == Int2())
		{
			path cookedAssetPath;// = path();
			if (GetCookedAssetPath(a_relativeFilePath, cookedAssetPath))
			{
				if (ReadSizeFromTexture(cookedAssetPath, g_importedTextureSizes[assetIndex]) == PUG_RESULT_OK)
				{
					res = &g_importedTextureSizes[assetIndex];
				}
			}
		}
		else
		{
			res = &g_importedTextureSizes[assetIndex];
		}
	}
	return res;
}