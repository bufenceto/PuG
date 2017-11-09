#include "asset_librarian.h"
#include "macro.h"
#include "vorpal_typedef.h"
#include "load_funcs.h"
#include "graphics.h"
#include "transform.h"
#include "material.h"
#include "mesh.h"

#include "logger/logger.h"
#include "utility/hash.h"
#include "utility/path.h"
#include "asset_processor/asset_types.h"

#include <cassert>
#include <experimental/filesystem>
#include <fstream>

using namespace std::experimental::filesystem;
using namespace std;
using namespace vpl::resource;
using namespace vpl::log;
using namespace vpl::utility;
using namespace vpl::graphics;
using namespace vpl;

#define MAX_ASSETS 2048

#define ASSET_ENTRY_AVAILABLE(i) (g_loadedAssetEntries[i].type == 0)
#define MESH_INDEX_AVAILABLE(i) ((g_meshes[i].vertices == INVALID_ID) && (g_meshes[i].indices == INVALID_ID))
#define TEXTURE_INDEX_AVAILABLE(i) ((g_textures[i] == INVALID_ID))
#define MATERIAL_INDEX_AVAILABLE(i) ((g_materials[i].isInitialized == 0))
#define MESH_VALID(m) ((m.vertices != INVALID_ID) && (m.indices != INVALID_ID))

//entries written by the asset cook tool
struct LibraryAssetEntry
{
	char assetGUID[SHA1_HASH_BYTES];//20 bytes
	uint32_t type;
	char extension[8];
};//32 bytes, hmmm alignment *drool*

static bool isInitialized = false;
static /*VPL_ALIGN(16)*/ Material g_materials[MAX_ASSETS];
//
static /*VPL_ALIGN(16)*/ Transform g_meshOffsets[MAX_ASSETS];
//
static /*VPL_ALIGN(16)*/ Mesh g_meshes[MAX_ASSETS];
//
static /*VPL_ALIGN(16)*/ TextureID g_textures[MAX_ASSETS];
//
static /*VPL_ALIGN(32)*/ Asset g_loadedAssetEntries[MAX_ASSETS * ((uint32_t)EAssetType::NumAssetTypes - 1)];
//static uint32_t g_loadedAssetCount;
//
static /*VPL_ALIGN(32)*/ LibraryAssetEntry g_assetLibrary[MAX_ASSETS];
static uint32_t g_assetLibraryEntriesCount;
//
static path g_currPath;

EAssetType ConvertType(uint32_t type)
{
	switch (type)
	{
	case 1: return EAssetType::Mesh;
	case 2: return EAssetType::Texture;
	default: return EAssetType::Unknown;
	}
}

void DeleteEntry(uint32_t index)
{

}

uint32_t FindAvailableAssetEntryIndex()
{
	uint32_t index = 0;
	for (uint32_t i = 0; i < VPL_COUNT_OF(g_loadedAssetEntries); ++i)
	{
		if (i != INVALID_ID && ASSET_ENTRY_AVAILABLE(i))
		{
			index = i;
			break;
		}
	}
	return index;
}

uint32_t FindAssetEntryIndexWithHash(const char* hash, size_t hashSize)
{
	uint32_t foundIndex = -1;
	for (int32_t i = 0; (uint32_t)i < g_assetLibraryEntriesCount; ++i)
	{
		if (memcmp(g_assetLibrary[i].assetGUID, hash, sizeof(hash)) == 0)
		{//entry found
			foundIndex = i;
			break;
		}
	}

	return foundIndex;
}

uint32_t FindMeshIndex()
{
	uint32_t index = 0;
	for (uint32_t i = 0; i < MAX_ASSETS; ++i)
	{
		if (i != INVALID_ID && MESH_INDEX_AVAILABLE(i))
		{
			index = i;
			break;
		}
	}
	return index;
}

uint32_t FindTextureIndex()
{
	uint32_t index = 0;
	for (uint32_t i = 0; i < VPL_COUNT_OF(g_textures); ++i)
	{
		if (i != INVALID_ID && TEXTURE_INDEX_AVAILABLE(i))
		{
			index = i;
			break;
		}
	}
	return index;
}

uint32_t FindMaterialIndex()
{
	uint32_t index = 0;
	for (uint32_t i = 0; i < VPL_COUNT_OF(g_materials); ++i)
	{
		if (i != INVALID_ID && MATERIAL_INDEX_AVAILABLE(i))
		{
			index = i;
			break;
		}
	}
	return index;
}

/*
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
*/

uint32_t ScanForAssetInDefaultPaths(std::experimental::filesystem::path& relativeAssetPath)
{
	const static path defaultTexturePath = g_currPath / "/../library/" / "textures";
	const static path defaultMeshPath = g_currPath / "/../library/" / "mesh";
	int32_t libraryEntryIndex = 0;

	//check absolute path


	//scan default texture path
	path fallbackPath = canonical(defaultTexturePath / relativeAssetPath.filename());
	if (exists(fallbackPath))
	{
		path hashString = "textures" / relativeAssetPath.filename();
		char fallbackHash[SHA1_HASH_BYTES];
		utility::SHA1(hashString.string(), fallbackHash, sizeof(fallbackHash));
		libraryEntryIndex = FindAssetEntryIndexWithHash(fallbackHash, sizeof(fallbackHash));
		if (libraryEntryIndex != -1)
		{//modify the string
			relativeAssetPath = hashString;
		}
	}
	/*
	directory_iterator it = directory_iterator(defaultTexturePath);
	for (directory_iterator end = directory_iterator(); it != end; ++it)
	{
		directory_entry curr = *it;
		if (!is_directory(curr))
		{//this is a file
			path fileNameStem = curr.path().filename().stem();
			path relativeAssetPathStem = relativeAssetPath.filename().stem();
			if (fileNameStem == relativeAssetPathStem)
			{//if stems match we still have to determine if the asset is of the correct type
				path foundFallbackPath = "textures/" / curr.path().filename();
				char fallbackHash[SHA1_HASH_BYTES];
				utility::SHA1(foundFallbackPath.string(), fallbackHash, sizeof(fallbackHash));
				libraryEntryIndex = FindAssetEntryIndexWithHash(fallbackHash, sizeof(fallbackHash));
				if (libraryEntryIndex != -1)
				{//modify the string
					relativeAssetPath = foundFallbackPath;
					break;
				}
			}
		}
		else
		{
			Warning("Do not put folders in the default directory, they will not be scanned for files!");
		}
	}
	*/
	return libraryEntryIndex;
}

RESULT ImportTextureAsset(const std::experimental::filesystem::path& absoluteCookedAssetPath, const char* hash, size_t hashSize)
{
	uint32_t assetIndex = FindAvailableAssetEntryIndex();
	Asset& entry = g_loadedAssetEntries[assetIndex];
	uint32_t index = FindTextureIndex();
	if (index != INVALID_ID)
	{
		uint8_t* data = nullptr;
		uint8_t* textureData = nullptr;
		DDS_HEADER header = {};
		uint64_t dataSize = 0;
		VPL_TRY(LoadDDSTexture(absoluteCookedAssetPath, data, textureData, dataSize, header));
		if (data != nullptr)
		{
			TextureID result = INVALID_ID;
			VPL_TRY(CreateTextureFromDDS(textureData, dataSize, header, result));
			if (result != INVALID_ID)
			{
				g_textures[index] = result;
			}
		}
		else
		{
			Error("Failed to load texture data!");
		}
		VPL_TRY(UnloadTexture(data))
	}
	else
	{
		Error("No more room in texture array!");
		return RESULT_ARRAY_FULL;
	}

	//write hash to AssetEntry
	memcpy(entry.guid, hash, hashSize);
	//write ID to AssetEntry
	entry.id = index;
	//write type to AssetEntry
	entry.type = (uint32_t)EAssetType::Texture;
	//++g_loadedAssetCount;

	return RESULT_OK;
}

RESULT ImportMeshAsset(const std::experimental::filesystem::path& absoluteCookedAssetPath, const char* hash, size_t hashSize, const std::experimental::filesystem::path& relativeCookedAssetPath = "")
{
	//load raw data from file
	Vertex** vertices = nullptr;
	uint32_t** indices = nullptr;
	uint32_t* vertexCount = 0;
	uint32_t* indexCount = 0;
	uint32_t meshCount = 0;
	RawMeshMaterial* rawMaterials = nullptr;
	VPL_TRY(LoadMesh(absoluteCookedAssetPath, vertices, vertexCount, indices, indexCount, rawMaterials, meshCount));
	//import mesh data
	for (uint32_t i = 0; i < meshCount; ++i)
	{
		uint32_t assetIndex = FindAvailableAssetEntryIndex();
		if (assetIndex != INVALID_ID)
		{
			Asset& entry = g_loadedAssetEntries[assetIndex];

			uint32_t index = FindMeshIndex();
			if (vertices[i] != nullptr && indices[i] != nullptr)
			{
				
				if (index != INVALID_ID)
				{
					//upload to the gpu
					VertexBufferID vb = INVALID_ID;
					IndexBufferID ib = INVALID_ID;
					VPL_TRY(CreateVertexBuffer(vertices[i], sizeof(vertices[i][0]), vertexCount[i], vb));
					VPL_TRY(CreateIndexBuffer(indices[i], sizeof(indices[i][0]), indexCount[i], ib));
					//if succes
					if ((vb != INVALID_ID) && (ib != INVALID_ID))
					{
						//write result to mesh array
						g_meshes[index].vertices = vb;
						g_meshes[index].indices = ib;
						g_meshes[index].vertexCount = vertexCount[i];
						g_meshes[index].indexCount = indexCount[i];
					}
				}
				else
				{
					Error("No more room in mesh array!");
					return RESULT_ARRAY_FULL;
				}
			}
			else
			{
				Error("Failed to load mesh data, not uploaded to the GPU!");
			}
			//write hash to AssetEntry
			memcpy(entry.guid, hash, hashSize);
			//write ID to AssetEntry
			entry.id = index;
			//write type to AssetEntry
			entry.type = (uint32_t)EAssetType::Mesh;
			//++g_loadedAssetCount;
		}
		else
		{
			return RESULT_ARRAY_FULL;
		}
	}
	//import material data
	for (uint32_t i = 0; i < meshCount; ++i)
	{
		uint32_t assetIndex = FindAvailableAssetEntryIndex();
		if (assetIndex != INVALID_ID)
		{
			Asset& entry = g_loadedAssetEntries[assetIndex];
			entry.type = (uint32_t)EAssetType::Material;
			uint32_t index = FindMaterialIndex();

			TextureID diffuse = 0;
			TextureID specular = 0;
			TextureID normal = 0;
			TextureID emissive = 0;
			if (rawMaterials[i].diffuseTexturePath.string().length() > 0)
			{
				const std::experimental::filesystem::path texturePath = MakeRelativeCanonical(relativeCookedAssetPath.parent_path() / rawMaterials[i].diffuseTexturePath);
				VPL_TRY(LoadAsset(texturePath));
				VPL_TRY(GetTextureAsset(texturePath, diffuse));
			}
			if (rawMaterials[i].specularTexturePath.string().length() > 0)
			{
				const std::experimental::filesystem::path texturePath = MakeRelativeCanonical(relativeCookedAssetPath.parent_path() / rawMaterials[i].specularTexturePath);
				VPL_TRY(LoadAsset(texturePath));
				VPL_TRY(GetTextureAsset(texturePath, specular));
			}
			if (rawMaterials[i].normalTexturePath.string().length() > 0)
			{
				const std::experimental::filesystem::path texturePath = MakeRelativeCanonical(relativeCookedAssetPath.parent_path() / rawMaterials[i].normalTexturePath);
				VPL_TRY(LoadAsset(texturePath));
				VPL_TRY(GetTextureAsset(texturePath, normal));
			}
			if (rawMaterials[i].emissiveTexturePath.string().length() > 0)
			{
				const std::experimental::filesystem::path texturePath = MakeRelativeCanonical(relativeCookedAssetPath.parent_path() / rawMaterials[i].emissiveTexturePath);
				VPL_TRY(LoadAsset(texturePath));
				VPL_TRY(GetTextureAsset(texturePath, emissive));
			}

			Material* mat = &g_materials[index];
			mat->diffuse = diffuse;
			mat->normal = normal;
			mat->isInitialized = 1;

			//write hash to AssetEntry
			memcpy(entry.guid, hash, hashSize);
			//write ID to AssetEntry
			entry.id = index;
			//write type to AssetEntry
			entry.type = (uint32_t)EAssetType::Material;
			//++g_loadedAssetCount;
		}
	}
	//import positional data


	//delete cpu data
	VPL_TRY(UnloadMesh(vertices, vertexCount, indices, indexCount, rawMaterials, meshCount));
	

	return RESULT_OK;
}

RESULT vpl::resource::InitAssetLibrarian()
{
	isInitialized = true;
	g_currPath = current_path();

	//look for library folder in "executable folder/../library"
	path libraryPath = canonical(g_currPath / "../library/");

	uint32_t numEntries = 0;
	uint32_t entrySize = 0;
	//import library from file
	if (exists(libraryPath))
	{
		fstream libraryFileStream;
		libraryFileStream.open(libraryPath / "asset_library.mal", fstream::in | fstream::binary);
		if (!libraryFileStream.is_open())
		{
			Error("Failed to open library file!");
			return RESULT_FAILED_TO_OPEN_FILE;
		}
		if (!libraryFileStream.read((char*)&numEntries, sizeof(uint32_t)))
		{
			Error("Failed to read library file header for entry size!\n");
			return RESULT_FAILED_TO_READ_FILE;
		}
		if (!libraryFileStream.read((char*)&entrySize, sizeof(uint32_t)))
		{
			Error("Failed to read library file header for entry count!\n");
			return RESULT_FAILED_TO_READ_FILE;
		}
		VPL_ASSERT(numEntries < MAX_ASSETS, "The number of entries in the mal file exceeds our maximum allowed asset count!");
		if (!libraryFileStream.read((char*)g_assetLibrary, numEntries * entrySize))
		{
			Error("Failed to read asset entries from library file !\n");
		}
	}
	else
	{
		Error("directory %s does not exist!", libraryPath.string().c_str());
		return RESULT_LIBRARY_DIRECTORY_NOT_FOUND;
	}

	g_assetLibraryEntriesCount = numEntries;
	Info("Finished importing asset library from %s", libraryPath.string().c_str());
	return RESULT_OK;
}

RESULT vpl::resource::ClearAssetLibrarian()
{
	isInitialized = false;
	for (uint32_t i = 0; i < MAX_ASSETS; ++i)
	{//unload loaded meshes
		///<TODO>
		ReleaseMeshAsset(g_meshes[i]);
		ReleaseTextureAsset(g_textures[i]);
	}
	
	VPL_ZERO_MEM(g_loadedAssetEntries);
	//g_loadedAssetCount = 0;
	VPL_ZERO_MEM(g_assetLibrary);
	g_assetLibraryEntriesCount = 0;
	VPL_ZERO_MEM(g_meshes);

	return RESULT_OK;
}

RESULT vpl::resource::LoadAsset(std::experimental::filesystem::path relativeAssetPath)
{
	VPL_ASSERT(isInitialized, "Asset librarian is not initialized!");
	
	//find the asset in the library file
	char hash[SHA1_HASH_BYTES];
	utility::SHA1(relativeAssetPath.string(), hash, sizeof(hash));
	int32_t libraryEntryIndex = FindAssetEntryIndexWithHash(hash, sizeof(hash));
	if (libraryEntryIndex == -1)
	{
		//fallback, check default directories
		Warning("Asset not found for path %s", relativeAssetPath.string());
		libraryEntryIndex = ScanForAssetInDefaultPaths(relativeAssetPath);
		if (libraryEntryIndex == -1)
		{//if we still have not found it
			return RESULT_ASSET_NOT_FOUND_IN_LIBRARY;
		}
		else
		{
			Info("Found asset in fallback path %s", relativeAssetPath.string());
		}
	}

	//determine type from library entry
	EAssetType type = ConvertType(g_assetLibrary[libraryEntryIndex].type);
	//construct absolute cooked asset path
	path absoluteCookedAssetPath = canonical(g_currPath / "/../library/" / relativeAssetPath);
	absoluteCookedAssetPath.replace_extension(g_assetLibrary[libraryEntryIndex].extension);
	//load asset using correct loader
	if (type == EAssetType::Mesh)
	{
		ImportMeshAsset(absoluteCookedAssetPath, hash, sizeof(hash), relativeAssetPath);
	}
	else if (type == EAssetType::Texture)
	{
		ImportTextureAsset(absoluteCookedAssetPath, hash, sizeof(hash));
	}
	else if (type == EAssetType::Unknown)
	{
		Error("Failed to determine asset type! Path: %s", absoluteCookedAssetPath.string().c_str());
	}
	else
	{
		Error("Invalid asset type! Path: %s", absoluteCookedAssetPath.string().c_str());
	}

	return RESULT_OK;
}

RESULT vpl::resource::UnloadAsset(
	const path& relativeAssetPath)
{
	VPL_ASSERT(isInitialized, "Asset librarian is not initialized!");
	return RESULT_OK;
}

RESULT vpl::resource::GetMeshAsset(
	const path& relativeAssetPath,
	Mesh* out_meshes,
	Material* out_materials,
	Transform* out_meshOffsets,
	uint32_t maxMeshCount,
	uint32_t& out_meshCount)
{
	char hash[SHA1_HASH_BYTES] = {};
	SHA1(relativeAssetPath.string(), hash, sizeof(hash));

	uint32_t assetFound = 0;
	uint32_t meshCounter = 0;
	uint32_t materialCounter = 0;
	uint32_t transformCounter = 0;
	

	//no early out, we can have multiple meshes and assets with the same hash
	for (uint32_t i = 1; i < VPL_COUNT_OF(g_loadedAssetEntries); ++i)
	{
		if (memcmp(hash, g_loadedAssetEntries[i].guid, sizeof(hash)) == 0)
		{
			assetFound = 1;
			if (g_loadedAssetEntries[i].type == (uint32_t)EAssetType::Mesh)
			{
				out_meshes[meshCounter] = g_meshes[g_loadedAssetEntries[i].id];
				++meshCounter;
				if (meshCounter > maxMeshCount)
				{
					return RESULT_ARRAY_FULL;
				}
			}
			else if (g_loadedAssetEntries[i].type == (uint32_t)EAssetType::Material)
			{
				out_materials[materialCounter] = g_materials[g_loadedAssetEntries[i].id];
				++materialCounter;
				if (materialCounter > maxMeshCount)
				{
					return RESULT_ARRAY_FULL;
				}
			}
		}
	}

	if (meshCounter != materialCounter)
	{
		Warning("Not all loaded meshes from file %s had materials assigned to them", relativeAssetPath.string().c_str());
	}

	if (assetFound == 0)
	{
		Error("Asset %s was not found", relativeAssetPath.string().c_str());
	}

	if (meshCounter == 0)
	{
		Error("No meshes were loaded from mesh file");
		return RESULT_UNKNOWN;
	}

	out_meshCount = meshCounter;
	return RESULT_OK;
}

RESULT vpl::resource::GetMeshAsset(
	const path& relativeAssetPath, 
	vpl::graphics::Mesh& out_result,
	vpl::graphics::Material& out_material)
{
	out_result = {};
	char hash[SHA1_HASH_BYTES] = {};
	SHA1(relativeAssetPath.string(), hash, sizeof(hash));

	for (uint32_t i = 0; i < VPL_COUNT_OF(g_loadedAssetEntries); ++i)
	{
		if (memcmp(hash, g_loadedAssetEntries[i + 1].guid, sizeof(hash)) == 0 &&
			g_loadedAssetEntries[i + 1].type == (uint32_t)EAssetType::Mesh)
		{//found loaded asset
			uint32_t meshIndex = g_loadedAssetEntries[i + 1].id;
			out_result = g_meshes[meshIndex];
			out_material = g_materials[meshIndex];
			return RESULT_OK;
		}
	}
	Error("Loaded asset not found! path: %s", relativeAssetPath.string().c_str());
	return RESULT_ASSET_NOT_LOADED;
}

RESULT vpl::resource::ReleaseMeshAsset(
	vpl::graphics::Mesh& meshAsset)
{
	if (MESH_VALID(meshAsset))
	{
		VPL_TRY(graphics::DestroyVertexBuffer(meshAsset.vertices));
		VPL_TRY(graphics::DestroyIndexBuffer(meshAsset.indices));
		meshAsset.vertices = INVALID_ID;
		meshAsset.indices = INVALID_ID;
		meshAsset.vertexCount = 0;
		meshAsset.indexCount = 0;
		return RESULT_OK;
	}
	else
	{
		return RESULT_INVALID_ARGUMENTS;
	}
}

RESULT vpl::resource::GetTextureAsset(
	const path& relativeAssetPath,
	TextureAssetID& out_result)
{
	out_result = {};
	char hash[SHA1_HASH_BYTES] = {};
	SHA1(relativeAssetPath.string(), hash, sizeof(hash));

	for (uint32_t i = 0; i < VPL_COUNT_OF(g_loadedAssetEntries); ++i)
	{
		if (memcmp(hash, g_loadedAssetEntries[i + 1].guid, sizeof(hash)) == 0 && 
			g_loadedAssetEntries[i + 1].type == (uint32_t)EAssetType::Texture)
		{//found loaded asset
			uint32_t texureAssetIndex = g_loadedAssetEntries[i + 1].id;
			out_result = g_textures[texureAssetIndex];
			return RESULT_OK;
		}
	}
	//asset not found in asset library, attempting absolute path fall back
	
	Warning("Loaded asset not found! path: %s", relativeAssetPath.string().c_str());
	return RESULT_ASSET_NOT_LOADED;
}

RESULT vpl::resource::DereferenceTextureAssetID(
	const vpl::resource::TextureAssetID textureAssetID,
	vpl::graphics::TextureID& out_result)
{
	return g_textures[textureAssetID];
}

RESULT vpl::resource::ReleaseTextureAsset(
	vpl::resource::TextureAssetID& textureAsset)
{
	if (g_textures[textureAsset] != INVALID_ID)
	{
		VPL_TRY(graphics::DestroyTexture(g_textures[textureAsset]));
		g_textures[textureAsset] = INVALID_ID;
		return RESULT_OK;
	}
	else
	{
		return RESULT_INVALID_ARGUMENTS;
	}
}