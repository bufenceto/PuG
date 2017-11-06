#include "mesh_converter.h"
#include "logger.h"

#include "assimp/Importer.hpp"
#include "assimp/Exporter.hpp"
#include "assimp/postprocess.h"

#include "Assimp/DefaultLogger.hpp"
#include "Assimp/Logger.hpp"

#include <experimental\filesystem>

using namespace vpl;
using namespace vpl::log;
using namespace Assimp;
using namespace std::experimental::filesystem;

MeshConverter::MeshConverter()
	: m_importer(new Importer())
	, m_exporter(new Exporter())
{

}

MeshConverter::~MeshConverter()
{
	delete m_exporter;
	delete m_importer;
}

bool MeshConverter::IsExtensionSupported(const path& extension) const
{
	return m_importer->IsExtensionSupported(extension.string());
}

uint32_t MeshConverter::CookAsset(
	const path& absoluteRawAssetInputPath, 
	const path& absoluteCookedAssetOutputPath) const
{
	uint32_t postProcessingFlags = 0;
	postProcessingFlags |= aiProcess_CalcTangentSpace;
	postProcessingFlags |= aiProcess_JoinIdenticalVertices;
	postProcessingFlags |= aiProcess_ConvertToLeftHanded;
	postProcessingFlags |= aiProcess_Triangulate;
	postProcessingFlags |= aiProcess_RemoveComponent;
	postProcessingFlags |= aiProcess_GenNormals;
	postProcessingFlags |= aiProcess_ValidateDataStructure;
	postProcessingFlags |= aiProcess_ImproveCacheLocality;
	postProcessingFlags |= aiProcess_FixInfacingNormals;
	postProcessingFlags |= aiProcess_FindDegenerates;
	postProcessingFlags |= aiProcess_FindInvalidData;
	postProcessingFlags |= aiProcess_GenUVCoords;
	postProcessingFlags |= aiProcess_FindInstances;
	postProcessingFlags |= aiProcess_OptimizeMeshes;
	postProcessingFlags |= aiProcess_OptimizeGraph;

	if (!m_importer->ValidateFlags(postProcessingFlags))
	{
		Log("Unsupported post-processing flags!");
		return RESULT_FAILED;
	}

	//Assimp::DefaultLogger::create(log::GetLogFilePath().c_str());

	const aiScene* scene = m_importer->ReadFile(absoluteRawAssetInputPath.string(), postProcessingFlags);
	if (!scene)
	{
		Log("Failed to read mesh file from path: %s", absoluteRawAssetInputPath.string());
		return RESULT_FAILED;
	}

	if (m_exporter->Export(scene, "assbin", absoluteCookedAssetOutputPath.string()) == aiReturn::aiReturn_FAILURE)
	{
		Log("Failed to export mesh to path: %s", absoluteCookedAssetOutputPath.string());
		return RESULT_FAILED;
	}

	//Assimp::DefaultLogger::delete();

	return RESULT_OK;
}