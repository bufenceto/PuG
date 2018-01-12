#include "load.h"
#include "macro.h"
#include "mesh.h"

#include "logger.h"

#include <experimental/filesystem>
#include <fstream>

#include "assimp/Importer.hpp"
#include "assimp/scene.h"

#define INVALID_ID 0
#define VERTICES_PER_FACE 3

using namespace pug;
using namespace pug::assets;
using namespace pug::assets::resource;
using namespace pug::assets::graphics;
using namespace pug::log;
using namespace std::experimental::filesystem;
using namespace std;
using namespace Assimp;
using namespace vmath;

struct SceneNode
{
	vmath::Matrix4 transform;//offset
	uint32_t materialIndex;
	uint32_t meshIndex;
};

void ParseSceneGraph(const aiNode* current, const Matrix4& currentMatrix, const aiScene* scene, vector<SceneNode>& out_result)
{
	uint32_t numChildren = current->mNumChildren;
	for (uint32_t i = 0; i < numChildren; ++i)
	{
		aiNode* currChild = current->mChildren[i];
		const Matrix4 nodeMatrix = (*(Matrix4*)&currChild->mTransformation) * currentMatrix;
		ParseSceneGraph(currChild, currentMatrix, scene, out_result);
	}

	for (uint32_t i = 0; i < current->mNumMeshes; ++i)
	{
		SceneNode node;
		node.transform = currentMatrix;
		node.meshIndex = current->mMeshes[i];
		node.materialIndex = scene->mMeshes[current->mMeshes[i]]->mMaterialIndex;
		out_result.push_back(node);
	}

}

PUG_RESULT pug::assets::resource::LoadMesh(
	const path& meshPath,
	Vertex**& out_vertices,
	uint32_t*& out_vertexCount,
	uint32_t**& out_indices,
	uint32_t*& out_indexCount,
	RawMeshMaterial*& out_rawMaterials,
	uint32_t& out_meshCount)
{
	if (!exists(meshPath))
	{
		Error("File does not exist!");
		return PUG_RESULT_FILE_DOES_NOT_EXIST;
	}

	Importer importer;
	const aiScene* scene = importer.ReadFile(meshPath.string(), 0);
	if (scene == nullptr)
	{
		return PUG_RESULT_FAILED_TO_READ_FILE;
	}

	//VPL_ASSERT(scene->mNumMeshes == 1, "We only support mesh files with 1 mesh!");

	//Vertex** verticesArray = (Vertex**)_aligned_malloc(sizeof(Vertex*) * scene->mNumMeshes, 32);
	//uint32_t* vertexCountArray = (uint32_t*)_aligned_malloc(sizeof(uint32_t) * scene->mNumMeshes, 32);
	//uint32_t** indicesArray = (uint32_t**)_aligned_malloc(sizeof(uint32_t*) * scene->mNumMeshes, 32);
	//uint32_t* indexCountArray = (uint32_t*)_aligned_malloc(sizeof(uint32_t) * scene->mNumMeshes, 32);

	//temp stack allocations
	Vertex** tmpVerticesArray = (Vertex**)_malloca(sizeof(Vertex*) * scene->mNumMeshes);
	uint32_t* tmpVertexCountArray = (uint32_t*)_malloca(sizeof(uint32_t) * scene->mNumMeshes);
	uint32_t** tmpIndicesArray = (uint32_t**)_malloca(sizeof(uint32_t*) * scene->mNumMeshes);
	uint32_t* tmpIndexCountArray = (uint32_t*)_malloca(sizeof(uint32_t) * scene->mNumMeshes);

	memset(tmpVerticesArray, 0, sizeof(Vertex*) * scene->mNumMeshes);
	memset(tmpVertexCountArray, 0, sizeof(uint32_t) * scene->mNumMeshes);
	memset(tmpIndicesArray, 0, sizeof(uint32_t*) * scene->mNumMeshes);
	memset(tmpIndexCountArray, 0, sizeof(uint32_t) * scene->mNumMeshes);

	for (uint32_t i = 0; i < scene->mNumMeshes; ++i)
	{
		aiMesh* mesh = scene->mMeshes[i];
		Vertex* vertices = (Vertex*)_aligned_malloc(sizeof(Vertex) * mesh->mNumVertices, 32);
		uint32_t* indices = (uint32_t*)_aligned_malloc(sizeof(uint32_t) * mesh->mNumFaces * VERTICES_PER_FACE, 32);//all meshes are triangulated

		uint32_t meshHasUVs = (mesh->GetNumUVChannels() >= 1 && mesh->mNumUVComponents[0] >= 2);

		//VPL_ASSERT(mesh->GetNumUVChannels() >= 1, "We only support meshes that has atleast 1 uv channel");
		//VPL_ASSERT(mesh->mNumUVComponents[0] >= 2, "We only support meshes with atleast 2 uv coordinates");
		for (uint32_t i = 0; i < mesh->mNumVertices; ++i)
		{
			Vector3 pos = Vector3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
			Vector3 normal = Vector3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
			Vector2 uv = Vector2(0);
			if (meshHasUVs)
			{
				uv = Vector2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
			}

			Vector3 tangent = Vector3(0);
			if (mesh->HasTangentsAndBitangents())
			{//prevent reading from empty tangent array
				tangent = Vector3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
			}

			vertices[i] =
			{
				pos,
				normal,
				tangent,
				uv,
			};
		}

		for (uint32_t i = 0; i < mesh->mNumFaces; ++i)
		{
			indices[i * VERTICES_PER_FACE + 0] = mesh->mFaces[i].mIndices[0];
			indices[i * VERTICES_PER_FACE + 1] = mesh->mFaces[i].mIndices[1];
			indices[i * VERTICES_PER_FACE + 2] = mesh->mFaces[i].mIndices[2];
		}

		tmpVerticesArray[i] = vertices;
		tmpIndicesArray[i] = indices;
		tmpVertexCountArray[i] = mesh->mNumVertices;
		tmpIndexCountArray[i] = mesh->mNumFaces * VERTICES_PER_FACE;
	}

	RawMeshMaterial* tmpRawMaterialArray = (RawMeshMaterial*)_malloca(sizeof(RawMeshMaterial) * scene->mNumMaterials);//allocate enough to generate all materials in the file, will filter later

	for (uint32_t i = 0; i < scene->mNumMaterials; ++i)
	{
		aiMaterial* material = scene->mMaterials[i];

		aiColor3D diffuse, ambient, emissive, specular;
		float shininess, opacity, shininessStrength;

		material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse);
		material->Get(AI_MATKEY_COLOR_AMBIENT, ambient);
		material->Get(AI_MATKEY_COLOR_SPECULAR, specular);
		material->Get(AI_MATKEY_COLOR_EMISSIVE, emissive);
		material->Get(AI_MATKEY_SHININESS, shininess);
		material->Get(AI_MATKEY_OPACITY, opacity);
		material->Get(AI_MATKEY_SHININESS_STRENGTH, shininessStrength);

		tmpRawMaterialArray[i].ambient = Vector4(1.0f);
		tmpRawMaterialArray[i].diffuse = Vector4(diffuse.r, diffuse.g, diffuse.b, 1.0f);
		tmpRawMaterialArray[i].specular = Vector4(specular.r, specular.g, specular.b, 1.0f) * Vector4(shininessStrength, shininessStrength, shininessStrength, 1.0f);
		tmpRawMaterialArray[i].emissive = Vector4(emissive.r, emissive.g, emissive.b, 1.0f);

		new (&tmpRawMaterialArray[i].diffuseTexturePath) path();
		new (&tmpRawMaterialArray[i].specularTexturePath) path();
		new (&tmpRawMaterialArray[i].normalTexturePath) path();
		new (&tmpRawMaterialArray[i].emissiveTexturePath) path();

		aiString texturePath;
		for (size_t tt = (size_t)(aiTextureType_DIFFUSE); tt < (size_t)(aiTextureType_UNKNOWN); ++tt)
		{
			for (size_t t = 0; t < material->GetTextureCount((aiTextureType)tt); ++t)
			{
				material->GetTexture((aiTextureType)tt, (unsigned int)t, &texturePath);
				switch (tt)
				{
				case (size_t)aiTextureType_DIFFUSE:
					tmpRawMaterialArray[i].diffuseTexturePath = texturePath.C_Str();
					tmpRawMaterialArray[i].diffuse = Vector4(1);
					break;
				case (size_t)aiTextureType_SPECULAR:
					tmpRawMaterialArray[i].specularTexturePath = texturePath.C_Str();
					tmpRawMaterialArray[i].specular = Vector4(1);
					break;
				case (size_t)aiTextureType_NORMALS:
				case (size_t)aiTextureType_HEIGHT:
					tmpRawMaterialArray[i].normalTexturePath = texturePath.C_Str();
					break;
				case (size_t)aiTextureType_EMISSIVE:
					tmpRawMaterialArray[i].emissiveTexturePath = texturePath.C_Str();
					tmpRawMaterialArray[i].emissive = Vector4(1);
					break;
				case (size_t)aiTextureType_AMBIENT:
					//log::Info("Loading of ambient maps is not supported!");
					break;
				default:
					log::Warning("Unsupported texture type! path: %s", texturePath.C_Str());
					break;
				}
			}
		}
	}

	//SceneNode* tmpSceneNodes = (SceneNode*)alloca(sizeof(SceneNode*) * scene->mNumMeshes);
	vector<SceneNode> tmpSceneNodes;
	Matrix4 rootTransform = Matrix4();
	ParseSceneGraph(scene->mRootNode, rootTransform, scene, tmpSceneNodes);

	//put temp arrays in final arrays
	Vertex** verticesArray = new Vertex*[scene->mNumMeshes];
	uint32_t* vertexCountArray = new uint32_t[scene->mNumMeshes];
	uint32_t** indicesArray = new uint32_t*[scene->mNumMeshes];
	uint32_t* indexCountArray = new uint32_t[scene->mNumMeshes];

	if (scene->mNumMaterials > scene->mNumMeshes)
	{
		Warning("More materials than meshes were loaded from file!");
	}
	RawMeshMaterial* rawMaterialArray = new RawMeshMaterial[scene->mNumMeshes];//create a material for each meshes, there should not be more materials than meshes!

																			   //loop over all meshes
	for (uint32_t i = 0; i < scene->mNumMeshes; ++i)
	{
		uint32_t meshIndex = tmpSceneNodes[i].meshIndex;
		verticesArray[meshIndex] = tmpVerticesArray[i];
		vertexCountArray[meshIndex] = tmpVertexCountArray[i];
		indicesArray[meshIndex] = tmpIndicesArray[i];
		indexCountArray[meshIndex] = tmpIndexCountArray[i];

		uint32_t materialIndex = tmpSceneNodes[i].materialIndex;
		PUG_ASSERT(materialIndex < scene->mNumMaterials, "Reading out of bounds");

		rawMaterialArray[i].diffuse = tmpRawMaterialArray[materialIndex].diffuse;
		rawMaterialArray[i].ambient = tmpRawMaterialArray[materialIndex].ambient;
		rawMaterialArray[i].specular = tmpRawMaterialArray[materialIndex].specular;
		rawMaterialArray[i].emissive = tmpRawMaterialArray[materialIndex].emissive;

		rawMaterialArray[i].diffuseTexturePath = tmpRawMaterialArray[materialIndex].diffuseTexturePath;
		rawMaterialArray[i].emissiveTexturePath = tmpRawMaterialArray[materialIndex].emissiveTexturePath;
		rawMaterialArray[i].normalTexturePath = tmpRawMaterialArray[materialIndex].normalTexturePath;
		rawMaterialArray[i].specularTexturePath = tmpRawMaterialArray[materialIndex].specularTexturePath;
	}

	out_vertices = verticesArray;
	out_vertexCount = vertexCountArray;
	out_indices = indicesArray;
	out_indexCount = indexCountArray;
	out_rawMaterials = rawMaterialArray;
	out_meshCount = scene->mNumMeshes;

	_freea(tmpVerticesArray);
	_freea(tmpVertexCountArray);
	_freea(tmpIndicesArray);
	_freea(tmpIndexCountArray);

	for (uint32_t i = 0; i < scene->mNumMaterials; ++i)
	{
		tmpRawMaterialArray[i].diffuseTexturePath.~path();
		tmpRawMaterialArray[i].specularTexturePath.~path();
		tmpRawMaterialArray[i].normalTexturePath.~path();
		tmpRawMaterialArray[i].emissiveTexturePath.~path();
	}

	_freea(tmpRawMaterialArray);

	return PUG_RESULT_OK;
}

PUG_RESULT pug::assets::resource::UnloadMesh(
	Vertex** vertices,
	uint32_t* vertexCounts,
	uint32_t** indices,
	uint32_t* indexCounts,
	RawMeshMaterial* rawMaterials,
	const uint32_t meshCount)
{
	for (uint32_t i = 0; i < meshCount; ++i)
	{
		if (vertices[i] != nullptr)
		{
			_aligned_free(vertices[i]);
			vertexCounts[i] = 0;
			vertices[i] = nullptr;
		}
		if (indices != nullptr)
		{
			_aligned_free(indices[i]);
			indexCounts[i] = 0;
			vertices[i] = nullptr;
		}
	}

	if (rawMaterials != nullptr)
	{
		delete[] rawMaterials;
		rawMaterials = nullptr;
	}
	delete[] vertices;
	vertices = nullptr;
	delete[] indices;
	indices = nullptr;
	delete[] vertexCounts;
	delete[] indexCounts;

	return PUG_RESULT_OK;
}

PUG_RESULT pug::assets::resource::LoadDDSTexture(
	const std::experimental::filesystem::path& path,
	uint8_t*& out_data,
	uint8_t*& out_textureData,
	uint64_t& out_dataSize,
	DDS_HEADER& out_header)
{
	if (!exists(path) || is_directory(path))
	{
		Error("File does not exist!");
		return PUG_RESULT_FILE_DOES_NOT_EXIST;
	}

	uint64_t fileSize = file_size(path);
	// Need at least enough data to fill the header and magic number to be a valid DDS
	if (fileSize < (sizeof(DDS_HEADER) + sizeof(uint32_t)))
	{
		Error("File is to small, can not contain a valid DDS file");
		return PUG_RESULT_INVALID_ARGUMENTS;
	}

	fstream ddsFile;
	ddsFile.open(path, fstream::in | fstream::binary);

	out_data = (uint8_t*)_aligned_malloc(fileSize, 16);
	uint32_t bytesRead = (uint32_t)ddsFile.read((char*)out_data, (uint32_t)fileSize).gcount();
	if (bytesRead != fileSize)
	{
		_aligned_free(out_data);
		return PUG_RESULT_FAILED_TO_READ_FILE;
	}

	// DDS files always start with the same magic number ("DDS ")
	uint32_t dwMagicNumber = *(const uint32_t*)(out_data);
	if (dwMagicNumber != DDS_MAGIC)
	{
		//memory leak!
		Error("Invalid dds file, magic number not present!\n");
		_aligned_free(out_data);
		return PUG_RESULT_INVALID_ARGUMENTS;
	}

	DDS_HEADER* hdr = reinterpret_cast<DDS_HEADER*>(out_data + sizeof(uint32_t));

	// Verify header to validate DDS file
	if (hdr->size != sizeof(DDS_HEADER) ||
		hdr->ddspf.size != sizeof(DDS_PIXELFORMAT))
	{
		Error("Invalid dds file, invalid header size!\n");
		_aligned_free(out_data);
		return PUG_RESULT_INVALID_ARGUMENTS;
	}

	// Check for DX10 extension
	//THIS SHOULD NEVER HAPPEN IN PUG!
	if ((hdr->ddspf.flags & DDS_FOURCC) && (VPL_MAKEFOURCC('D', 'X', '1', '0') == hdr->ddspf.fourCC))
	{
		Error("Invalid DDS file type!");
		_aligned_free(out_data);
		return PUG_RESULT_INVALID_ARGUMENTS;//we do not support DXT10
	}

	// setup the pointers in the process request
	out_header = *hdr;
	ptrdiff_t offset = sizeof(uint32_t) + sizeof(DDS_HEADER);
	out_data = out_data;
	out_textureData = out_data + offset;
	out_dataSize = fileSize - offset;

	return PUG_RESULT_OK;
}

PUG_RESULT pug::assets::resource::UnloadTexture(
	uint8_t* data)
{
	if (data != nullptr)
	{
		_aligned_free(data);
		return PUG_RESULT_OK;
	}
	return PUG_RESULT_INVALID_ARGUMENTS;
}