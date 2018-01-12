#include "mesh_converter.h"
#include "logger.h"
#include "sha1.h"
#include "app_utility.h"

#include "database/asset_database.h"

#include "assimp/Importer.hpp"
#include "assimp/Exporter.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"

#include "Assimp/DefaultLogger.hpp"
#include "Assimp/Logger.hpp"

#include <experimental\filesystem>

#include <Windows.h>

using namespace pug;
using namespace pug::assets;
using namespace pug::log;
using namespace Assimp;

using std::experimental::filesystem::path;

void ImportMeshDependencies(const aiScene* a_scene, const path& relativeParentPath)
{
	path g_assetBasePath = GetAssetBasePath();

	for(uint32_t i = 0; i < a_scene->mNumMaterials; ++i)
	{
		aiMaterial* mat = a_scene->mMaterials[i];
		for (uint32_t texType = (uint32_t)(aiTextureType_DIFFUSE); texType < (uint32_t)(aiTextureType_UNKNOWN); ++texType)
		{
			//get number of textures for this type
			uint32_t texCount = mat->GetTextureCount((aiTextureType)texType);
			for (uint32_t j = 0; j < texCount; ++j)
			{
				aiString aiTexturePath;
				mat->Get(_AI_MATKEY_TEXTURE_BASE, texType, j, aiTexturePath);
				Info("Importing Asset Dependency from %s", aiTexturePath.C_Str());
				
				path relativeDependentAssetPath;
				MakePathRelativeToAssetBasePath(relativeParentPath / path(aiTexturePath.C_Str()), g_assetBasePath, relativeDependentAssetPath);
				ImportAsset(SHA1Hash(relativeDependentAssetPath.string().c_str()), relativeDependentAssetPath, AssetType_Texture);
			}
		}
	}
}

void DeimportMeshDependencies(const aiScene* a_scene, const path& relativeParentPath)
{
	path g_assetOutputPath = GetAssetOutputPath();

	for (uint32_t i = 0; i < a_scene->mNumMaterials; ++i)
	{
		aiMaterial* mat = a_scene->mMaterials[i];
		for (uint32_t texType = (uint32_t)(aiTextureType_DIFFUSE); texType < (uint32_t)(aiTextureType_UNKNOWN); ++texType)
		{
			//get number of textures for this type
			uint32_t texCount = mat->GetTextureCount((aiTextureType)texType);
			for (uint32_t j = 0; j < texCount; ++j)
			{
				aiString aiTexturePath;
				mat->Get(_AI_MATKEY_TEXTURE_BASE, texType, j, aiTexturePath);
				Info("Removing Asset Dependency from %s", aiTexturePath.C_Str());

				path relativeDependentAssetPath;
				MakePathRelativeToAssetBasePath(relativeParentPath / path(aiTexturePath.C_Str()), g_assetOutputPath, relativeDependentAssetPath);
				RemoveAsset(SHA1Hash(relativeDependentAssetPath.string().c_str()), relativeDependentAssetPath);
			}
		}
	}
}

MeshConverter::MeshConverter()
	//: m_importer(new Importer())
	//, m_exporter(new Exporter())
{
	//InitializeCriticalSection(&m_activeFileConversionListGuard);
}

MeshConverter::~MeshConverter()
{
	//DeleteCriticalSection(&m_activeFileConversionListGuard);
	//delete m_exporter;
	//delete m_importer;
}

bool MeshConverter::IsExtensionSupported(const path& extension) const
{
	Assimp::Importer* imp = new Assimp::Importer();
	
	bool isSupported = imp->IsExtensionSupported(extension.string());
	delete imp;
	
	return isSupported;
}

PUG_RESULT MeshConverter::CookAsset(
	const path& absoluteRawAssetInputPath, 
	const path& absoluteCookedAssetOutputPath,
	const AssetSettings a_assetSettings)
{
	PUG_RESULT res = PUG_RESULT_OK;

	uint32_t postProcessingFlags = 0;
	postProcessingFlags |= aiProcess_CalcTangentSpace			* a_assetSettings.m_meshSettings.m_calcTangents;
	postProcessingFlags |= aiProcess_JoinIdenticalVertices		* a_assetSettings.m_meshSettings.m_mergeIdenticalVertices;
	postProcessingFlags |= aiProcess_ConvertToLeftHanded		* a_assetSettings.m_meshSettings.m_convertToLeftHanded;
	postProcessingFlags |= aiProcess_Triangulate				* a_assetSettings.m_meshSettings.m_triangulate;
	postProcessingFlags |= aiProcess_RemoveComponent;			
	postProcessingFlags |= aiProcess_GenNormals					* a_assetSettings.m_meshSettings.m_generateNormals;
	postProcessingFlags |= aiProcess_ValidateDataStructure		* a_assetSettings.m_meshSettings.m_validateData;
	postProcessingFlags |= aiProcess_ImproveCacheLocality		* a_assetSettings.m_meshSettings.m_improveCacheLocality;
	postProcessingFlags |= aiProcess_FixInfacingNormals			* a_assetSettings.m_meshSettings.m_fixInfacingNormals;
	postProcessingFlags |= aiProcess_FindDegenerates			* a_assetSettings.m_meshSettings.m_removeDegenerateTriangles;
	postProcessingFlags |= aiProcess_FindInvalidData			* a_assetSettings.m_meshSettings.m_validateData;
	postProcessingFlags |= aiProcess_GenUVCoords				* a_assetSettings.m_meshSettings.m_generateUVCoordinates;
	postProcessingFlags |= aiProcess_FindInstances				* a_assetSettings.m_meshSettings.m_removeDuplicateMeshEntriesFromSceneGraph;
	postProcessingFlags |= aiProcess_OptimizeMeshes				* a_assetSettings.m_meshSettings.m_optimizeMeshes;
	postProcessingFlags |= aiProcess_OptimizeGraph				* a_assetSettings.m_meshSettings.m_optimizeSceneGraph;

	Assimp::Importer* importer = new Assimp::Importer();
	if (!importer->ValidateFlags(postProcessingFlags))
	{
		Log("Unsupported post-processing flags!");
		return PUG_RESULT_INVALID_ARGUMENTS;
	}

	uint32_t extensionMask = !(absoluteRawAssetInputPath.extension() == ".assbin");//if this is an assbin file we mask away our post process flags
	const aiScene* scene = importer->ReadFile(absoluteRawAssetInputPath.string(), postProcessingFlags * extensionMask);
	if (scene)
	{
		Assimp::Exporter* exporter = new Assimp::Exporter();
		if (exporter->Export(scene, "assbin", absoluteCookedAssetOutputPath.string()) == aiReturn::aiReturn_FAILURE)
		{
			Log("Failed to export mesh to path: %s", absoluteCookedAssetOutputPath.string());
			res = PUG_RESULT_FAILED_TO_WRITE_FILE;
		}

		path parentBase = absoluteRawAssetInputPath.parent_path();
		ImportMeshDependencies(scene, parentBase);

		delete exporter;
	}
	else
	{
		Log("Failed to read mesh file from path: %s", absoluteRawAssetInputPath.string());
		res = PUG_RESULT_FAILED_TO_READ_FILE;
	}

	delete importer;

	return res;
}

PUG_RESULT MeshConverter::UncookAsset(
	const path& absoluteCookedAssetPath)
{
	if (absoluteCookedAssetPath.extension() != ".assbin")
	{
		Error("Tried to uncook a raw mesh file");
		return PUG_RESULT_INVALID_ARGUMENTS;
	}

	Assimp::Importer* importer = new Assimp::Importer();

	//we must be only reading from an assbin file
	const aiScene* scene = importer->ReadFile(absoluteCookedAssetPath.string(), 0);
	if (scene)
	{
		path parentBase = absoluteCookedAssetPath.parent_path();
		DeimportMeshDependencies(scene, parentBase);
	}

	//delete output file
	//path absoluteAssetOutputPath = g_assetOutputPath / path(a_assetPath).replace_extension(a_assetEntry.m_extension);
	if (exists(absoluteCookedAssetPath))
	{
		if (DeleteFile(absoluteCookedAssetPath.string().c_str()) == 0)
		{
			Error("Failed to delete outputted asset file at: %s", absoluteCookedAssetPath.string().c_str());
			return PUG_RESULT_FAILED_TO_READ_FILE;
		}
	}

	return PUG_RESULT_OK;
}

/*
void MeshConverter::WaitForActiveConversion(const SHA1Hash& a_assetHash)
{
	EnterCriticalSection(&m_activeFileConversionListGuard);

	int32_t index = -1;
	for (int32_t i = 0; i < (int32_t)m_activeFileConversions.size(); ++i)
	{
		if (m_activeFileConversions[i].first == a_assetHash)
		{
			index = i;
			break;
		}
	}

	LeaveCriticalSection(&m_activeFileConversionListGuard);
}
*/