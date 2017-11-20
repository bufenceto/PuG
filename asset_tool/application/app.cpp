#include "../inc/app_intf.h"

#include "importers/mesh_converter.h"
#include "importers/texture_converter.h"
#include "sha1.h"
#include "asset_option_flags.h"

#include "graphics.h"
#include "load.h"
#include "camera.h"
#include "gui.h"
#include "input.h"

#include "imgui/imgui.h"

#include <experimental\filesystem>
#include <vector>

#include <Windows.h>

#define MAX_PATH_SIZE 260

class Application : public pug::iApp
{
public:
	Application();
	~Application();

	bool Initialize() override;
	bool Destroy() override;

	bool Update(const float deltaTime) override;

	void GetRenderData(
		pug::assets::graphics::Mesh*& out_meshes,
		pug::assets::graphics::Transform*& out_transforms,
		pug::assets::graphics::Material*&,
		uint32_t& out_meshCount) override;
	void GetMainCamera(
		pug::assets::graphics::Camera& out_camera,
		vmath::Vector3& out_position,
		vmath::Quaternion& out_rotation) override;
	void GetLightData(
		pug::assets::graphics::PointLight*& out_pointLights,
		uint32_t& out_pointLightCount,
		pug::assets::graphics::DirectionalLight*& out_directionalLights,
		vmath::Vector4*& out_directionalLightShadowMapBoxes,
		uint32_t& out_directionalLightCount) override;
};

using namespace pug;
using namespace pug::assets;
using namespace pug::assets::resource;
using namespace pug::assets::graphics;
using namespace pug::input;

using namespace vmath;
using namespace std;
using namespace std::experimental::filesystem;

struct TextureSettings
{
	TEXTURE_COMPRESSION_METHODS m_compressionMethod;
	uint8_t m_compress;
	uint8_t m_makePowerOfTwo;
	uint8_t m_compressWithoutAlpha;
};

struct MeshSettings
{
	uint8_t m_calcTangents;
	uint8_t m_mergeIdenticalVertices;
	uint8_t m_generateNormals;
	uint8_t m_validateData;
	uint8_t m_improveCacheLocality;
	uint8_t m_removeDegenerateTriangles;
	uint8_t m_generateUVCoordinates;
	uint8_t m_removeDuplicateMeshEntriesFromSceneGraph;
	uint8_t m_fixInfacingNormals;
	uint8_t m_triangulate;
	uint8_t m_convertToLeftHanded;
	uint8_t m_optimizeMeshes;
	uint8_t m_optimizeSceneGraph;
};

struct AssetSettings
{
	AssetType type;
	union
	{
		TextureSettings texSettings;
		MeshSettings meshSettings;
	};
};

struct TextureOptions
{
	uint32_t textureOptions;
	uint32_t compressionMethod;
};

struct MeshOptions
{
	uint32_t meshOptions;
};

struct Asset
{
	AssetType m_typ;
	char m_relativeAssetPath[260];
	union
	{
		TextureOptions textureOptions;
		MeshOptions meshOptions;
	};
};

pug::iApp* pug::iApp::Create()
{
	return new Application();
}

Camera mainCamera;
Transform cameraTransform;

Mesh cubeMesh;
Transform cubeTransform;

path g_assetBasePath;

static string g_currentSelected = "";

static AssetConverter* g_converters[] =
{
	new MeshConverter(),
	new TextureConverter(),
};

vector<std::pair<SHA1Hash, Asset>> g_assetCodex;//the thing that will actually be written to the file
vector<std::pair<SHA1Hash, AssetSettings>> g_assetSettings;

ImVec2 operator+ (const ImVec2& a, const ImVec2& b)
{
	return ImVec2(a.x + b.x, a.y + b.y);
}
ImVec2 operator* (const ImVec2& a, const ImVec2& b)
{
	return ImVec2(a.x * b.x, a.y * b.y);
}
ImVec2 operator* (const ImVec2& a, const float b)
{
	return ImVec2(a.x * b, a.y * b);
}

void ImportAsset(const SHA1Hash& a_assetHash, AssetType a_assetType)
{
	AssetSettings settings = {};
	settings.type = a_assetType;
	g_assetSettings.push_back(std::pair<SHA1Hash, AssetSettings>(a_assetHash, settings));
}

AssetType DetermineAssetType(const path& assetPath)
{
	PUG_ASSERT(assetPath.has_extension(), "no extensions detected!");
	for (uint32_t i = 0; i < PUG_COUNT_OF(g_converters); ++i)
	{
		if (g_converters[i]->IsExtensionSupported(assetPath.extension()))
		{
			return g_converters[i]->GetAssetType();
		}
	}
}

void NewAssetSelected(const path& newAssetPath)
{
	g_currentSelected = newAssetPath.string();
}

void ImGuiBuildMeshDetails(const SHA1Hash& a_assetHash)
{
	ImGui::Text("Mesh Asset:");

	int32_t assetIndex = -1;
	for (int32_t i = 0; i < (int32_t)g_assetSettings.size(); ++i)
	{
		if (g_assetSettings[i].first == a_assetHash)
		{
			assetIndex = i;
		}
	}
	if (assetIndex != -1)
	{//asset was present
		PUG_ASSERT(g_assetSettings[assetIndex].second.type == AssetType_Mesh, "Asset Type Incorrect!");
		MeshSettings& settings = g_assetSettings[assetIndex].second.meshSettings;

		if (ImGui::TreeNode("Missing Data"))
		{
			ImGui::PushStyleVar(ImGuiStyleVar_ChildWindowRounding, 5.0f);
			ImGui::BeginChild("Sub1", ImVec2(0, 80), true);
			ImGui::Checkbox("Calculate Vertex Normals", (bool*)&settings.m_generateNormals);
			ImGui::Checkbox("Calculate Vertex Tangents", (bool*)&settings.m_calcTangents);
			ImGui::Checkbox("Calculate UV Coordinates", (bool*)&settings.m_generateUVCoordinates);
			ImGui::EndChild();
			ImGui::PopStyleVar();

			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Mesh Fixes"))
		{
			ImGui::PushStyleVar(ImGuiStyleVar_ChildWindowRounding, 5.0f);
			ImGui::BeginChild("Sub2", ImVec2(0, 125), true);
			ImGui::Checkbox("Merge Identical Vertices", (bool*)&settings.m_mergeIdenticalVertices);
			ImGui::Checkbox("Remove Degenerate Triangles", (bool*)&settings.m_removeDegenerateTriangles);
			ImGui::Checkbox("Fix Infacing Normals", (bool*)&settings.m_fixInfacingNormals);
			ImGui::Checkbox("Triangulate", (bool*)&settings.m_triangulate);
			ImGui::Checkbox("Convert To Left-Handed", (bool*)&settings.m_convertToLeftHanded);
			ImGui::EndChild();
			ImGui::PopStyleVar();

			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Scene Fixes"))
		{
			ImGui::PushStyleVar(ImGuiStyleVar_ChildWindowRounding, 5.0f);
			ImGui::BeginChild("Sub3", ImVec2(0, 105), true);
			ImGui::Checkbox("Improve Cache Locality", (bool*)&settings.m_improveCacheLocality);
			ImGui::Checkbox("Remove Duplicate Mesh Entries", (bool*)&settings.m_removeDuplicateMeshEntriesFromSceneGraph);
			ImGui::Checkbox("Optimize Meshes", (bool*)&settings.m_optimizeMeshes);
			ImGui::Checkbox("Optimize Scene Graph", (bool*)&settings.m_optimizeSceneGraph);
			ImGui::EndChild();
			ImGui::PopStyleVar();

			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Misc"))
		{
			ImGui::PushStyleVar(ImGuiStyleVar_ChildWindowRounding, 5.0f);
			ImGui::BeginChild("Sub3", ImVec2(0, 35), true);
			ImGui::Checkbox("Validate Data", (bool*)&settings.m_validateData);
			ImGui::EndChild();
			ImGui::PopStyleVar();

			ImGui::TreePop();
		}
	}
	else
	{
		if (ImGui::Button("Import this asset"))
		{
			ImportAsset(a_assetHash, AssetType_Mesh);
		}
	}
}

void ImGuiBuildTextureDetails(const SHA1Hash& assetHash)
{
	ImGui::Text("Texture Asset:");

}

void ImGuiBuildDirectoryStructure()
{//asset repository folder structure visualization 
	ImGui::Begin("Asset Repository");
	vector<directory_iterator> directoryIteratorStack;

	directoryIteratorStack.push_back(directory_iterator(g_assetBasePath));
	uint32_t treeNodeCount = 0;
	if (ImGui::TreeNode(canonical(g_assetBasePath).string().c_str()))
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
				bool selected = (currDir.path().string() == g_currentSelected);

				if (ImGui::Selectable(currDir.path().filename().string().c_str(), &selected))
				{
					NewAssetSelected(currDir.path().string());
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
}

void BuildGUI()
{
	//ImGui::Begin("Info");
	//	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", GetDeltaTime() * 1000.0f, 1.0f / GetDeltaTime());
	//ImGui::End();

	ImGui::ShowTestWindow();

	ImGuiBuildDirectoryStructure();

	{//selected assets details
		ImGui::Begin("Asset Details");
		if (g_currentSelected != "")
		{
			AssetType type = DetermineAssetType(g_currentSelected);
			switch (type)
			{
			case AssetType_Mesh: ImGuiBuildMeshDetails(SHA1Hash(g_currentSelected.c_str())); break;
			case AssetType_Texture: ImGuiBuildTextureDetails(SHA1Hash(g_currentSelected.c_str())); break;
			default:
				log::Warning("Asset type not recognized!"); break;
			}
		}
		ImGui::End();
	}
}

void Quit()
{
	log::Info("Quit Event!");
	Stop();
}

Application::Application()
{

}

Application::~Application()
{

}

bool Application::Initialize()
{
	Vertex** cubeVertices = nullptr;
	uint32_t* cubeVerticesCount = nullptr;

	uint32_t** cubeIndices = nullptr;
	uint32_t* cubeIndicesCount = nullptr;

	RawMeshMaterial* materials = nullptr;
	uint32_t meshCount = 0;

	LoadMesh("../assets/cube.assbin", cubeVertices, cubeVerticesCount, cubeIndices, cubeIndicesCount, materials, meshCount);

	for (uint32_t i = 0; i < meshCount; ++i)
	{
		CreateVertexBuffer(cubeVertices[i], sizeof(Vertex), cubeVerticesCount[i], cubeMesh.m_vbh);
		CreateIndexBuffer(cubeIndices[i], PUG_FORMAT_R32_UINT, cubeIndicesCount[i], cubeMesh.m_ibh);
	}

	cubeTransform =
	{
		Vector3(0, 0, 3),
		Quaternion(),
		Vector3(1),
	};

	mainCamera = 
	{
		RADIANS(60.0f),
		0.1f,
		1000.0f,
	};

	cameraTransform =
	{
		Vector3(0, 0, 0),
		Quaternion(),
		Vector3(1),
	};

	input::RegisterQuitEvent(Quit);
	input::RegisterKeyEvent(KEY_ID_Escape, INPUT_EVENT_RELEASED, Quit);

	char executablePath[MAX_PATH_SIZE];
	GetModuleFileNameA(GetModuleHandleA(NULL), executablePath, MAX_PATH_SIZE);
	//g_assetBasePath = canonical(path(executablePath).parent_path() / "../assets/");
	g_assetBasePath = "../assets/";

	//read asset repository file

	return true;
}

bool Application::Destroy()
{
	return false;
}

bool Application::Update(const float deltaTime)
{
	cubeTransform.rotation = cubeTransform.rotation * Quaternion(Vector3(RADIANS(10.0f)) * deltaTime);

	BuildGUI();

	return true;
}

void Application::GetRenderData(
	pug::assets::graphics::Mesh*& out_meshes,
	pug::assets::graphics::Transform*& out_transforms,
	pug::assets::graphics::Material*& out_material,
	uint32_t& out_meshCount)
{
	out_meshes = &cubeMesh;
	out_transforms = &cubeTransform;
	out_material = nullptr;
	out_meshCount = 1;
}

void Application::GetMainCamera(
	pug::assets::graphics::Camera& out_camera,
	vmath::Vector3& out_position,
	vmath::Quaternion& out_rotation)
{
	out_camera = mainCamera;
	out_position = cameraTransform.position;
	out_rotation = cameraTransform.rotation;
}

void Application::GetLightData(
	pug::assets::graphics::PointLight*& out_pointLights,
	uint32_t& out_pointLightCount,
	pug::assets::graphics::DirectionalLight*& out_directionalLights,
	vmath::Vector4*& out_directionalLightShadowMapBoxes,
	uint32_t& out_directionalLightCount)
{

}