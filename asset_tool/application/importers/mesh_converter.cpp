#include "mesh_converter.h"
#include "logger.h"

#include "database/asset_database.h"

#include "assimp/Importer.hpp"
#include "assimp/Exporter.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"

#include "Assimp/DefaultLogger.hpp"
#include "Assimp/Logger.hpp"

#include <experimental\filesystem>

using namespace pug;
using namespace pug::assets;
using namespace pug::log;
using namespace Assimp;
using namespace std::experimental::filesystem;

void ImportAssetDependencies(const aiScene* a_scene)
{
	//for (uint32_t i = 0; i < a_scene->mNumMeshes; ++i)
	//{
	//	a_scene->mMeshes[i]->
	//}

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
				ImportAsset(SHA1Hash(aiTexturePath.C_Str()), aiTexturePath.C_Str(), AssetType_Texture);
				//pugMaterial.AddTexture(TextureReference(aiTexturePath.C_Str()), ConvertaiTextureType((aiTextureType)texType));
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

uint32_t MeshConverter::CookAsset(
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

	uint32_t extensionMask = !(absoluteRawAssetInputPath.extension() == ".assbin");
	const aiScene* scene = importer->ReadFile(absoluteRawAssetInputPath.string(), postProcessingFlags * extensionMask);
	if (scene)
	{
		Assimp::Exporter* exporter = new Assimp::Exporter();
		if (exporter->Export(scene, "assbin", absoluteCookedAssetOutputPath.string()) == aiReturn::aiReturn_FAILURE)
		{
			Log("Failed to export mesh to path: %s", absoluteCookedAssetOutputPath.string());
			res = PUG_RESULT_FAILED_TO_WRITE_FILE;
		}
		ImportAssetDependencies(scene);

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