#include "mesh_reader.h"

#include "importers/mesh_converter.h"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"

using namespace pug;
using namespace pug::assets;
using namespace std::experimental::filesystem;
using namespace vmath;
using namespace Assimp;

Importer* importer = new Importer();

TEXTURE_TYPE ConvertaiTextureType(const aiTextureType texType)
{
	switch (texType)
	{
	case aiTextureType_NONE: return TEXTURE_TYPE_NONE;
	case aiTextureType_DIFFUSE: return TEXTURE_TYPE_DIFFUSE;
	case aiTextureType_SPECULAR: return TEXTURE_TYPE_SPECULAR;
	case aiTextureType_AMBIENT: return TEXTURE_TYPE_AMBIENT;
	case aiTextureType_EMISSIVE: return TEXTURE_TYPE_EMISSIVE;
	case aiTextureType_HEIGHT: return TEXTURE_TYPE_HEIGHT;
	case aiTextureType_NORMALS: return TEXTURE_TYPE_NORMALS;
	case aiTextureType_SHININESS: return TEXTURE_TYPE_SHININESS;
	case aiTextureType_OPACITY: return TEXTURE_TYPE_OPACITY;
	case aiTextureType_DISPLACEMENT: return TEXTURE_TYPE_DISPLACEMENT;
	case aiTextureType_LIGHTMAP: return TEXTURE_TYPE_LIGHTMAP;
	case aiTextureType_REFLECTION: return TEXTURE_TYPE_REFLECTION;
	case aiTextureType_UNKNOWN: return TEXTURE_TYPE_UNKNOWN;
	default:
		return TEXTURE_TYPE_UNKNOWN;
	}
}

PUG_RESULT pug::assets::ReadMaterialFromMesh(
	const std::experimental::filesystem::path& a_absoluteCookedAssetPath,
	pug::assets::Material& out_material)
{
	//does this file exist and is it a file
	if (!exists(a_absoluteCookedAssetPath) || is_directory(a_absoluteCookedAssetPath))
	{
		log::Error("File does not exist!");
		return PUG_RESULT_INVALID_ARGUMENTS;
	}
	//is this a valid file
	if (a_absoluteCookedAssetPath.extension() != COOKED_MESH_EXTENSION)
	{
		log::Error("File extension was invalid");
		return PUG_RESULT_INVALID_ARGUMENTS;
	}

	PUG_RESULT res = PUG_RESULT_UNKNOWN;
	//no import flags, we should be reading from a processed asset only!
	const aiScene* scene = importer->ReadFile(a_absoluteCookedAssetPath.string().c_str(), 0);
	if (scene != nullptr)
	{
		//PUG_ASSERT(scene->mNumMaterials <= 2, "more than 1 material detected, no support yet");
		uint32_t matIndex = scene->mMeshes[0]->mMaterialIndex;// scene->mNumMaterials > 1 ? 1 : 0;
		if (scene->mNumMaterials > 0)
		{
			Material pugMaterial = {};
			aiMaterial* assimpMaterial = scene->mMaterials[matIndex];

			aiColor3D aiDiffuse = aiColor3D(0);
			aiColor3D aiAmbient = aiColor3D(0);
			aiColor3D aiEmissive = aiColor3D(0);
			aiColor3D aiSpecular = aiColor3D(0);
			float aiShininess = 0.0f;
			float aiOpacity = 0.0f;
			float aiShininessStrength = 0.0f;

			aiString aiDiffuseTexturePath = aiString();
			aiString aiNormalTexturePath = aiString();
			aiString aiSpecularTexturePath = aiString();
			aiString aiEmissiveTexturePath = aiString();

			assimpMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, aiDiffuse);
			assimpMaterial->Get(AI_MATKEY_COLOR_AMBIENT, aiAmbient);
			assimpMaterial->Get(AI_MATKEY_COLOR_SPECULAR, aiSpecular);
			assimpMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, aiEmissive);
			assimpMaterial->Get(AI_MATKEY_SHININESS, aiShininess);
			assimpMaterial->Get(AI_MATKEY_OPACITY, aiOpacity);
			assimpMaterial->Get(AI_MATKEY_SHININESS_STRENGTH, aiShininessStrength);

			pugMaterial.SetDiffuseColor(Vector3(aiDiffuse.r, aiDiffuse.g, aiDiffuse.b));
			pugMaterial.SetAmbientColor(Vector3(aiAmbient.r, aiAmbient.g, aiAmbient.b));
			pugMaterial.SetSpecularColor(Vector3(aiSpecular.r, aiSpecular.g, aiSpecular.b));
			pugMaterial.SetEmissiveColor(Vector3(aiEmissive.r, aiEmissive.g, aiEmissive.b));
			pugMaterial.SetShininess(aiShininess);
			pugMaterial.SetOpacity(aiOpacity);
			pugMaterial.SetShininessStrength(aiShininessStrength);

			//for all texture types
			for (uint32_t texType = (uint32_t)(aiTextureType_DIFFUSE); texType < (uint32_t)(aiTextureType_UNKNOWN); ++texType)
			{
				//get number of textures for this type
				uint32_t texCount = assimpMaterial->GetTextureCount((aiTextureType)texType);
				for (uint32_t i = 0; i < texCount; ++i)
				{
					aiString aiTexturePath;
					assimpMaterial->Get(_AI_MATKEY_TEXTURE_BASE, texType, i, aiTexturePath);
					pugMaterial.AddTexture(TextureReference(aiTexturePath.C_Str()), ConvertaiTextureType((aiTextureType)texType));
				}
			}
			out_material = pugMaterial;
			res = PUG_RESULT_OK;
		}
	}
	else
	{
		res = PUG_RESULT_FAILED_TO_READ_FILE;
	}

	importer->FreeScene();
	return res;
}

/* FUNCTION WAS SCRAPPED
PUG_RESULT pug::assets::ReadDependenciesFromMesh(
	const path& a_absoluteCookedAssetPath,
	std::vector<Material>& out_materials)
{
	PUG_RESULT res = PUG_RESULT_UNKNOWN;
	//no import flags, we should be reading from a processed asset only!
	const aiScene* scene = importer->ReadFile(a_absoluteCookedAssetPath.string().c_str(), 0);
	if (scene != nullptr)
	{
		for (uint32_t i = 0; i < scene->mNumMaterials; ++i)
		{
			Material pugMaterial = {};
			aiMaterial* assimpMaterial = scene->mMaterials[i];

			aiColor3D aiDiffuse = aiColor3D(0);
			aiColor3D aiAmbient = aiColor3D(0);
			aiColor3D aiEmissive = aiColor3D(0);
			aiColor3D aiSpecular = aiColor3D(0);
			float aiShininess = 0.0f;
			float aiOpacity = 0.0f;
			float aiShininessStrength = 0.0f;

			aiString aiDiffuseTexturePath = aiString();
			aiString aiNormalTexturePath = aiString();
			aiString aiSpecularTexturePath = aiString();
			aiString aiEmissiveTexturePath = aiString();

			assimpMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, aiDiffuse);
			assimpMaterial->Get(AI_MATKEY_COLOR_AMBIENT, aiAmbient);
			assimpMaterial->Get(AI_MATKEY_COLOR_SPECULAR, aiSpecular);
			assimpMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, aiEmissive);
			assimpMaterial->Get(AI_MATKEY_SHININESS, aiShininess);
			assimpMaterial->Get(AI_MATKEY_OPACITY, aiOpacity);
			assimpMaterial->Get(AI_MATKEY_SHININESS_STRENGTH, aiShininessStrength);

			pugMaterial.SetDiffuseColor(Vector3(aiDiffuse.r, aiDiffuse.g, aiDiffuse.b));
			pugMaterial.SetAmbientColor(Vector3(aiAmbient.r, aiAmbient.g, aiAmbient.b));
			pugMaterial.SetSpecularColor(Vector3(aiSpecular.r, aiSpecular.g, aiSpecular.b));
			pugMaterial.SetEmissiveColor(Vector3(aiEmissive.r, aiEmissive.g, aiEmissive.b));
			pugMaterial.SetShininess(aiShininess);
			pugMaterial.SetOpacity(aiOpacity);
			pugMaterial.SetShininessStrength(aiShininessStrength);

			//for all texture types
			for (uint32_t texType = (uint32_t)(aiTextureType_DIFFUSE); texType < (uint32_t)(aiTextureType_UNKNOWN); ++texType)
			{
				//get number of textures for this type
				uint32_t texCount = assimpMaterial->GetTextureCount((aiTextureType)texType);
				for (uint32_t i = 0; i < texCount; ++i)
				{
					aiString aiTexturePath;
					assimpMaterial->Get(_AI_MATKEY_TEXTURE_BASE, texType, i, aiTexturePath);
					pugMaterial.AddTexture(TextureReference(aiTexturePath.C_Str()), ConvertaiTextureType((aiTextureType)texType));
				}
			}
			out_materials.push_back(pugMaterial);
			res = PUG_RESULT_OK;
		}
	}
	else
	{
		res = PUG_RESULT_FAILED_TO_READ_FILE;
	}

	importer->FreeScene();
	return res;
}
*/