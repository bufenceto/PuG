#include "../inc/app_intf.h"

#include "sha1.h"
#include "asset_option_flags.h"
#include "asset_types.h"
#include "app_utility.h"

#include "database/asset_database.h"

#include "gui/gui_builder.h"

#include "importers/asset_cooker.h"
#include "importers/asset_settings.h"
#include "directory_monitor/asset_directory_monitor.h"

#include "graphics.h"
#include "load.h"
#include "camera.h"
#include "gui.h"
#include "input.h"

#include "imgui/imgui.h"

#include <experimental\filesystem>
#include <vector>

#include <Windows.h>

#define RELATIVE_ASSET_DIR "../assets/"
#define RELATIVE_LIBRARY_DIR "../library/"
#define ASSET_DATABASE_FILENAME "asset_tool_data.adb"
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

//struct TextureOptions
//{
//	uint32_t textureOptions;
//	uint32_t compressionMethod;
//};
//
//struct MeshOptions
//{
//	uint32_t meshOptions;
//};

//struct Asset
//{
//	AssetType m_typ;
//	char m_relativeAssetPath[260];
//	union
//	{
//		TextureOptions textureOptions;
//		MeshOptions meshOptions;
//	};
//};

pug::iApp* pug::iApp::Create()
{
	return new Application();
}

Camera mainCamera;
Transform cameraTransform;

Mesh cubeMesh;
Transform cubeTransform;

path g_assetBasePath;
path g_assetOutputPath;
path g_assetSettingsFilePath;

//vector<SHA1Hash> g_importedAssets;
//vector<AssetSettings> g_importedAssetSettings;

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

path GetAssetBasePath()
{
	return g_assetBasePath;
}

path GetAssetOutputPath()
{
	return g_assetOutputPath;
}

void DirectoryChangeCallBack(DirectoryChange dirChange)
{
	log::Info("Directory monitor reported change in file %s", dirChange.fileName);
	AssetSettings assetSettings = {};
	if (FindAssetSettingsForFile(dirChange.fileName, assetSettings))
	{
		log::Info("Detected change was in an imported file");
		SubmitCookJob(dirChange.fileName, assetSettings, 1);
	}
}

void BuildGUI()
{
	//ImGui::Begin("Info");
	//	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", GetDeltaTime() * 1000.0f, 1.0f / GetDeltaTime());
	//ImGui::End();

	ImGui::ShowTestWindow();

	BuildDirectoryStructureWindow(g_assetBasePath);

	std::string selectedItemString = GetCurrentSelectedItemString();

	AssetType type = DetermineAssetType(selectedItemString);
	ITEM_DETAILS_GUI_RESULT res =
		BuildAssetDetailWindow(
			selectedItemString,
			type);

	SHA1Hash assetHash = selectedItemString.c_str();
	switch (res)
	{
	case ITEM_DETAILS_GUI_RESULT_IMPORT_ITEM:
	{
		ImportAsset(
			assetHash,
			selectedItemString,
			type);
		break;
	}
	case ITEM_DETAILS_GUI_RESULT_REMOVE_ITEM:
	{
		RemoveAsset(
			assetHash,
			selectedItemString);
		break;
	}
	case ITEM_DETAILS_GUI_RESULT_OPTION_CLICKED:
	{
		AssetSettings settings = {};
		uint32_t res = FindAssetSettingsForFile(assetHash, settings);
		PUG_ASSERT(res, "Asset not found!");
		SubmitCookJob(selectedItemString, settings);
		break;
	}
	default:
		;//
	}

	std::vector<path> activeJobPaths;
	GetCopyOfActiveJobList(activeJobPaths);
	BuildActiveJobWindow(activeJobPaths);

	std::vector<path> queuedJobPaths;
	GetCopyOfQueuedJobList(queuedJobPaths);
	BuildQueuedJobWindow(queuedJobPaths);

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
	g_assetBasePath = canonical(RELATIVE_ASSET_DIR);
	g_assetOutputPath = canonical(g_assetBasePath / RELATIVE_LIBRARY_DIR);

	char executablePath[MAX_PATH_SIZE];
	GetModuleFileName(GetModuleHandleA(NULL), executablePath, MAX_PATH_SIZE);
	path exePath = executablePath;
	g_assetSettingsFilePath = exePath.parent_path() / ASSET_DATABASE_FILENAME;
	if (!exists(g_assetBasePath))
	{
		if (!create_directories(g_assetBasePath))
		{
			log::Error("Failed to create asset directory at %s!", g_assetBasePath.string().c_str());
			return 1;
		}
	}

	PUG_TRY(CreateAssetDataBase(g_assetSettingsFilePath));

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

	UnloadMesh(cubeVertices, cubeVerticesCount, cubeIndices, cubeIndicesCount, materials, meshCount);
	

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

	PUG_TRY(InitAssetCooker(g_assetBasePath, g_assetOutputPath));
	PUG_TRY(InitAssetDirectoryMonitor(g_assetBasePath, DirectoryChangeCallBack));

	//g_assetBasePath = canonical(path(executablePath).parent_path() / "../assets/");
	
	//read asset repository file

	return true;
}

bool Application::Destroy()
{
	PUG_TRY(DestroyAssetDataBase());
	PUG_TRY(DestroyAssetDirectoryMonitor());
	PUG_TRY(DestroyAssetCooker());

	return true;
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