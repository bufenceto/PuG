#include "gui_builder.h"

#include "database/asset_database.h"

#include "logger.h"
#include "macro.h"

#include "app_utility.h"

#include "imgui/imgui.h"
#include "vmath/vmath.h"

using namespace vmath;
using namespace std;
using namespace std::experimental::filesystem;
using namespace pug::assets;
using namespace pug::log;

static string g_currentSelected = "";

string GetTextureTypeString(TEXTURE_TYPE a_texType)
{
	switch (a_texType)
	{
	case TEXTURE_TYPE_DIFFUSE: return "Diffuse";
	case TEXTURE_TYPE_SPECULAR: return "Specular";
	case TEXTURE_TYPE_AMBIENT: return "Ambient";
	case TEXTURE_TYPE_EMISSIVE: return "Emissive";
	case TEXTURE_TYPE_HEIGHT: return "Height";
	case TEXTURE_TYPE_NORMALS: return "Normal";
	case TEXTURE_TYPE_SHININESS: return "Shininess";
	case TEXTURE_TYPE_OPACITY: return "Opacity";
	case TEXTURE_TYPE_DISPLACEMENT: return "Displacement";
	case TEXTURE_TYPE_LIGHTMAP: return "Lightmap";
	case TEXTURE_TYPE_REFLECTION: return "Reflection";
	default:
		//log::Warning("Unrecognized Compression Method"); 
		return "UNKNOWN";
	}
}

string Vector3ToString(const Vector3& a_vector)
{
	stringstream res;
	res << "x: " << a_vector.x << " y: " << a_vector.y << +" z: " << a_vector.z << std::endl;

	return res.str();
}

bool ImGuiTextAndVec3Slider(const char* text, Vector3& vec3, const char* ID)
{
	float cursorY = ImGui::GetCursorPosY();
	ImGui::SetCursorPosY(cursorY + 3.0f);
	ImGui::Text(text);
	ImGui::SameLine();
	ImGui::SetCursorPosY(cursorY);
	ImGui::SetCursorPosX(80.0f);

	return ImGui::SliderFloat3(ID, &vec3.x, 0.0f, 1.0f);
}

static const char* GetCompressionMethodString(TEXTURE_COMPRESSION_METHODS method)
{
	switch (method)
	{
	case TEXTURE_COMPRESSION_METHODS_UNCOMPRESSED: return "UNCOMPRESSED";
	case TEXTURE_COMPRESSION_METHODS_BC1: return "BC1_UNORM";
	case TEXTURE_COMPRESSION_METHODS_BC2: return "BC2_UNORM";
	case TEXTURE_COMPRESSION_METHODS_BC3: return "BC3_UNORM";
	case TEXTURE_COMPRESSION_METHODS_BC4: return "BC4_UNORM";
	case TEXTURE_COMPRESSION_METHODS_BC5: return "BC5_UNORM";
	case TEXTURE_COMPRESSION_METHODS_BC6: return "BC6_UNORM";
	case TEXTURE_COMPRESSION_METHODS_BC7: return "BC7_UNORM";
	default:
		//log::Warning("Unrecognized Compression Method"); 
		return "UNKNOWN_COMPRESSION_METHOD";
	}
}

static bool ItemPresentInImportedList(const SHA1Hash& itemHash, const vector<SHA1Hash>& a_importedItems)
{
	for (uint32_t i = 0; i < (uint32_t)a_importedItems.size(); ++i)
	{
		if (a_importedItems[i] == itemHash)
		{
			return true;
		}
	}
	return false;
}

static int32_t FindItemInList(const SHA1Hash& itemHash, const vector<SHA1Hash>& a_importedItems)
{
	int32_t index = -1; 
	for (int32_t i = 0; i < (int32_t)a_importedItems.size(); ++i)
	{
		if (a_importedItems[i] == itemHash)
		{
			index = i;
			break;
		}
	}
	return index;
}

static ITEM_DETAILS_GUI_RESULT ImGuiBuildMeshDetails(
	const SHA1Hash& a_assetHash, 
	const path& a_relativeAssetPath,
	AssetSettings& a_importedItemSettings)
{
	ITEM_DETAILS_GUI_RESULT res = ITEM_DETAILS_GUI_RESULT_NOTHING;
	ImGui::Text((string("Mesh Asset ") + a_relativeAssetPath.filename().string() + ":").c_str());

	PUG_ASSERT(a_importedItemSettings.m_type == AssetType_Mesh, "Asset Type Incorrect!");
	MeshSettings& settings = a_importedItemSettings.m_meshSettings;
	if (ImGui::CollapsingHeader("Import Options"))
	{
		if (ImGui::TreeNode("Missing Data/Data Generation"))
		{
			ImGui::PushStyleVar(ImGuiStyleVar_ChildWindowRounding, 5.0f);
			ImGui::BeginChild("Sub1", ImVec2(0, 105), true);
			if (ImGui::Checkbox("Calculate Vertex Normals", (bool*)&settings.m_generateNormals))
			{
				res = ITEM_DETAILS_GUI_RESULT_OPTION_CLICKED;
			}
			if (ImGui::Checkbox("Calculate Vertex Tangents", (bool*)&settings.m_calcTangents))
			{
				res = ITEM_DETAILS_GUI_RESULT_OPTION_CLICKED;
			}
			if (ImGui::Checkbox("Calculate UV Coordinates", (bool*)&settings.m_generateUVCoordinates))
			{
				res = ITEM_DETAILS_GUI_RESULT_OPTION_CLICKED;
			}
			if (ImGui::SliderInt("LOD:", (int32_t*)&settings.m_numLODs, 0, 16))
			{
				res = ITEM_DETAILS_GUI_RESULT_OPTION_CLICKED;
			}
			ImGui::EndChild();
			ImGui::PopStyleVar();

			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Mesh Fixes"))
		{
			ImGui::PushStyleVar(ImGuiStyleVar_ChildWindowRounding, 5.0f);
			ImGui::BeginChild("Sub2", ImVec2(0, 125), true);
			if (ImGui::Checkbox("Merge Identical Vertices", (bool*)&settings.m_mergeIdenticalVertices))
			{
				res = ITEM_DETAILS_GUI_RESULT_OPTION_CLICKED;
			}
			if (ImGui::Checkbox("Remove Degenerate Triangles", (bool*)&settings.m_removeDegenerateTriangles))
			{
				res = ITEM_DETAILS_GUI_RESULT_OPTION_CLICKED;
			}
			if (ImGui::Checkbox("Fix Infacing Normals", (bool*)&settings.m_fixInfacingNormals))
			{
				res = ITEM_DETAILS_GUI_RESULT_OPTION_CLICKED;
			}
			if (ImGui::Checkbox("Triangulate", (bool*)&settings.m_triangulate))
			{
				res = ITEM_DETAILS_GUI_RESULT_OPTION_CLICKED;
			}
			if (ImGui::Checkbox("Convert To Left-Handed", (bool*)&settings.m_convertToLeftHanded))
			{
				res = ITEM_DETAILS_GUI_RESULT_OPTION_CLICKED;
			}
			ImGui::EndChild();
			ImGui::PopStyleVar();

			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Scene Fixes"))
		{
			ImGui::PushStyleVar(ImGuiStyleVar_ChildWindowRounding, 5.0f);
			ImGui::BeginChild("Sub3", ImVec2(0, 105), true);
			if (ImGui::Checkbox("Improve Cache Locality", (bool*)&settings.m_improveCacheLocality))
			{
				res = ITEM_DETAILS_GUI_RESULT_OPTION_CLICKED;
			}
			if (ImGui::Checkbox("Remove Duplicate Mesh Entries", (bool*)&settings.m_removeDuplicateMeshEntriesFromSceneGraph))
			{
				res = ITEM_DETAILS_GUI_RESULT_OPTION_CLICKED;
			}
			if (ImGui::Checkbox("Optimize Meshes", (bool*)&settings.m_optimizeMeshes))
			{
				res = ITEM_DETAILS_GUI_RESULT_OPTION_CLICKED;
			}
			if (ImGui::Checkbox("Optimize Scene Graph", (bool*)&settings.m_optimizeSceneGraph))
			{
				res = ITEM_DETAILS_GUI_RESULT_OPTION_CLICKED;
			}
			ImGui::EndChild();
			ImGui::PopStyleVar();

			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Misc"))
		{
			ImGui::PushStyleVar(ImGuiStyleVar_ChildWindowRounding, 5.0f);
			ImGui::BeginChild("Sub3", ImVec2(0, 35), true);
			if (ImGui::Checkbox("Validate Data", (bool*)&settings.m_validateData))
			{
				res = ITEM_DETAILS_GUI_RESULT_OPTION_CLICKED;
			}
			ImGui::EndChild();
			ImGui::PopStyleVar();

			ImGui::TreePop();
		}
	}
	if (ImGui::CollapsingHeader("Material"))
	{
		Material* material = FindMaterialForCookedMeshFile(a_relativeAssetPath);
		if(material != nullptr)
		{
			Vector3 diffuse = material->GetDiffuse();
			if (ImGuiTextAndVec3Slider("Diffuse: ", diffuse, "##Diffuse"))
			{
				material->SetDiffuseColor(diffuse);
			}
			Vector3 ambient = material->GetAmbient();
			if (ImGuiTextAndVec3Slider("Ambient: ", ambient, "##Ambient"))
			{
				material->SetAmbientColor(ambient);
			}
			Vector3 emissive = material->GetEmissive();
			if (ImGuiTextAndVec3Slider("Emissive: ", emissive, "##Emissive"))
			{
				material->SetEmissiveColor(emissive);
			}
			Vector3 specular = material->GetSpecular();
			if (ImGuiTextAndVec3Slider("Specular: ", specular, "##Specular"))
			{
				material->SetSpecularColor(specular);
			}

			for (uint32_t i = 0; i < TEXTURE_TYPE_COUNT; ++i)
			{
				uint32_t texCount = material->GetTexCountForType((TEXTURE_TYPE)i);

				for (uint32_t j = 0; j < texCount; ++j)
				{
					const TextureReference* texRefs = material->GetTexturesForType((TEXTURE_TYPE)i);
					stringstream stream;
					stream << GetTextureTypeString((TEXTURE_TYPE)i) << j << ": " << texRefs[j].GetReferencedPath().string();
					ImGui::Text(stream.str().c_str());
				}
			}

		}
	}
	if (ImGui::Button("Remove this mesh asset"))
	{
		//RemoveAsset(a_assetHash, a_relativeAssetPath);
		res = ITEM_DETAILS_GUI_RESULT_REMOVE_ITEM;
	}
	
	return res;
}

static ITEM_DETAILS_GUI_RESULT ImGuiBuildTextureDetails(
	const SHA1Hash& a_assetHash, 
	const path& a_relativeAssetPath,
	AssetSettings& a_importedItemSettings)
{
	ITEM_DETAILS_GUI_RESULT res = ITEM_DETAILS_GUI_RESULT_NOTHING;
	ImGui::Text((string("Mesh Asset ") + a_relativeAssetPath.filename().string() + ":").c_str());

	//PUG_ASSERT(g_assetSettings[assetIndex].second.m_type == AssetType_Texture, "Asset Type Incorrect!");
	PUG_ASSERT(a_importedItemSettings.m_type == AssetType_Texture, "Asset Type Incorrect!");
	TextureSettings& settings = a_importedItemSettings.m_texSettings;

	//TEXTURE_COMPRESSION_METHODS m_compressionMethod;
	TEXTURE_COMPRESSION_METHODS selectedCompressionMethod = settings.m_compressionMethod == 0 ? (TEXTURE_COMPRESSION_METHODS)1 : settings.m_compressionMethod;
	const char* compresString = GetCompressionMethodString(selectedCompressionMethod);
	if (ImGui::TreeNode(compresString))
	{
		if (ImGui::RadioButton("BC1 (Color Maps)", compresString == "TEXTURE_COMPRESSION_METHODS_BC1"))
		{
			settings.m_compressionMethod = TEXTURE_COMPRESSION_METHODS_BC1;
			res = ITEM_DETAILS_GUI_RESULT_OPTION_CLICKED;
		}
		if (ImGui::RadioButton("BC2 (N/A)", compresString == "TEXTURE_COMPRESSION_METHODS_BC2"))
		{
			settings.m_compressionMethod = TEXTURE_COMPRESSION_METHODS_BC2;
			res = ITEM_DETAILS_GUI_RESULT_OPTION_CLICKED;
		}
		if (ImGui::RadioButton("BC3 (Color maps with alpha)", compresString == "TEXTURE_COMPRESSION_METHODS_BC3"))
		{
			settings.m_compressionMethod = TEXTURE_COMPRESSION_METHODS_BC3;
			res = ITEM_DETAILS_GUI_RESULT_OPTION_CLICKED;
		}
		if (ImGui::RadioButton("BC4 (Grayscale)", compresString == "TEXTURE_COMPRESSION_METHODS_BC4"))
		{
			settings.m_compressionMethod = TEXTURE_COMPRESSION_METHODS_BC4;
			res = ITEM_DETAILS_GUI_RESULT_OPTION_CLICKED;
		}
		if (ImGui::RadioButton("BC5 (Tangent-space normal maps)", compresString == "TEXTURE_COMPRESSION_METHODS_BC5"))
		{
			settings.m_compressionMethod = TEXTURE_COMPRESSION_METHODS_BC5;
			res = ITEM_DETAILS_GUI_RESULT_OPTION_CLICKED;
		}
		if (ImGui::RadioButton("BC6 (HDR)", compresString == "TEXTURE_COMPRESSION_METHODS_BC6"))
		{
			settings.m_compressionMethod = TEXTURE_COMPRESSION_METHODS_BC6;
			res = ITEM_DETAILS_GUI_RESULT_OPTION_CLICKED;
		}
		if (ImGui::RadioButton("BC7 (HQ Color/Color with full alpha)", compresString == "TEXTURE_COMPRESSION_METHODS_BC7"))
		{
			settings.m_compressionMethod = TEXTURE_COMPRESSION_METHODS_BC7;
			res = ITEM_DETAILS_GUI_RESULT_OPTION_CLICKED;
		}
		if (ImGui::SliderInt("Mips: ", (int32_t*)&settings.m_numMipsToGenerate, 0, 16))
		{
			res = ITEM_DETAILS_GUI_RESULT_OPTION_CLICKED;
		}
		ImGui::TreePop();
	}
	//uint8_t m_compress;
	if (ImGui::Checkbox("Compress", (bool*)&settings.m_compress))
	{
		res = ITEM_DETAILS_GUI_RESULT_OPTION_CLICKED;
	}
	//uint8_t m_makePowerOfTwo;
	if (ImGui::Checkbox("Make Power Of 2", (bool*)&settings.m_makePowerOfTwo))
	{
		res = ITEM_DETAILS_GUI_RESULT_OPTION_CLICKED;
	}
	//uint8_t m_compressWithoutAlpha;
	if (ImGui::Checkbox("Compress Without Alpha", (bool*)&settings.m_compressWithoutAlpha))
	{
		res = ITEM_DETAILS_GUI_RESULT_OPTION_CLICKED;
	}
	//uint32_t m_numMipsToGenerate;
	if (ImGui::Button("Remove this texture asset"))
	{
		res = ITEM_DETAILS_GUI_RESULT_REMOVE_ITEM;
	}

	return res;
}

uint32_t pug::assets::BuildDirectoryStructureWindow(
	const path& a_assetBasePath)
{
	uint32_t res = 0;
	ImGui::Begin("Asset Repository");
	vector<directory_iterator> directoryIteratorStack;

	directoryIteratorStack.push_back(directory_iterator(a_assetBasePath));
	uint32_t treeNodeCount = 0;
	if (ImGui::TreeNode(canonical(a_assetBasePath).string().c_str()))
	{
		uint32_t stackEmpty = 0;
		directory_iterator end = directory_iterator();
		do
		{
			directory_entry currDir = *(directoryIteratorStack.back());
			if (is_directory(currDir))
			{
				if (ImGui::TreeNode(currDir.path().filename().string().c_str()))
				{
					++directoryIteratorStack.back();
					directoryIteratorStack.push_back(directory_iterator(currDir));
					continue;
				}
			}
			else
			{
				path relativePath = MakePathRelativeToAssetBasePath(currDir.path().string(), a_assetBasePath);
				bool selected = (relativePath.string() == g_currentSelected);

				path relPath = MakePathRelativeToAssetBasePath(currDir.path(), a_assetBasePath).string();// .string();

				string stringToShow = relPath.filename().string();
				if (IsItemInDatabase(relPath.string().c_str()))
				{
					//currDirToShow += " [imported]";
					stringToShow += " (+)";
				}
				if (ImGui::Selectable(stringToShow.c_str(), &selected))
				{
					g_currentSelected = relativePath.string();
					res = 1;
					//NewAssetSelected(currDir.path().string());
				}
			}

			++directoryIteratorStack.back();
			while (!directoryIteratorStack.empty() && directoryIteratorStack.back() == end)
			{//pop off iterators that have reached their end
				directoryIteratorStack.pop_back();
				ImGui::TreePop();
			}
		} while (!directoryIteratorStack.empty());
	}
	ImGui::End();

	return res;
}

ITEM_DETAILS_GUI_RESULT pug::assets::BuildAssetDetailWindow(
	const path& a_relativeAssetPath,
	const AssetType a_assetType)
{
	//PUG_ASSERT(a_importedItems.size() == inout_importedItemSettings.size(), "Lists out of sync");

	ITEM_DETAILS_GUI_RESULT res = ITEM_DETAILS_GUI_RESULT_NOTHING;
	ImGui::Begin("Asset Details");
	if (g_currentSelected != "")
	{

		SHA1Hash hash = SHA1Hash(a_relativeAssetPath.string().c_str());
		AssetSettings assetSettings = {}; // itemIsImported ? &inout_importedItemSettings[itemIndex] : nullptr;
		if(FindAssetSettingsForFile(hash, assetSettings))
		{
			switch (a_assetType)
			{
			case AssetType_Mesh:
			{
				res = ImGuiBuildMeshDetails(
					hash,
					a_relativeAssetPath,
					assetSettings);
				break;
			}
			case AssetType_Texture:
			{
				res = ImGuiBuildTextureDetails(
					hash,
					a_relativeAssetPath,
					assetSettings);
				break;
			}
			default:
				Warning("Asset type not recognized!"); break;
			}
			SetAssetSettingsForFile(hash, assetSettings);
		}
		else
		{
			if (ImGui::Button("Import this asset"))
			{
				//add the file to our asset settings list
				res = ITEM_DETAILS_GUI_RESULT_IMPORT_ITEM;
				//ImportAsset(a_assetHash, a_relativeAssetPath, AssetType_Mesh);
			}
		}
		
	}
	ImGui::End();

	return res;
}

void pug::assets::BuildActiveJobWindow(
	const std::vector<path>& a_activeJobPaths)
{
	ImGui::Begin("Active Jobs");
	for (uint32_t i = 0; i < (uint32_t)a_activeJobPaths.size(); ++i)
	{
		ImGui::Text(a_activeJobPaths[i].string().c_str());
	}
	ImGui::End();
}

const string pug::assets::GetCurrentSelectedItemString()
{
	return g_currentSelected;
}