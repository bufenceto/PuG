#include "core/inc/asset_processor.h"

#include "pug_funcs.h"

#include "core/inc/macro.h"
#include "core/inc/app_intf.h"

#include "graphics/inc/window.h"
#include "graphics/inc/graphics.h"
#include "graphics/inc/transform.h"
#include "graphics/inc/camera.h"

#include "time/inc/timer.h"

#include "input/inc/input.h"
#include "input/inc/input_monitor.h"

#include "gui/inc/gui.h"

#include <experimental/filesystem>

using namespace pug;
using namespace pug::platform;
using namespace pug::time;
using namespace vmath;

static iApp* g_app;

static Window* g_window;
static Timer g_timer;

static float deltaTime;
static uint8_t isRunning;

float pug::assets::GetDeltaTime()
{
	return deltaTime;
}

void pug::assets::Stop()
{
	isRunning = 0;
}

void pug::assets::InitAssetProcessor()
{
	log::StartLog(std::experimental::filesystem::current_path().string());

	g_window = Window::Create(Int2(1600, 900), "PuG Asset Processor", 0);
	PUG_ASSERT(g_window, "g_window invalid!");
	
	PUG_TRY(input::InitializeInput(g_window->IsMaximized(), g_window->GetSize()));
	PUG_TRY(graphics::InitGraphics(g_window, 0, 0));
	PUG_TRY(gui::InitGUI(g_window));

	g_app = iApp::Create();
	if (g_app != nullptr)
	{
		if (!g_app->Initialize())
		{
			log::Error("Failed to Init game!");
		}
	}

	isRunning = 1;
}

void pug::assets::RunAssetProcessor()
{
	static uint32_t count = 0;
	while (isRunning)
	{
		deltaTime = (float)g_timer.GetFrameDelta();
		
		PUG_TRY(g_window->Update());
		g_window->SetSize(input::GetWindowSize());

		PUG_TRY(gui::StartGUI(deltaTime, g_window));

		if (g_app != nullptr)
		{
			if (!g_app->Update(deltaTime))
			{
				log::Error("Failed to update app!");
			}
		}

		graphics::Mesh* guiMeshes = nullptr;
		graphics::Rect* scissorRects = nullptr;
		graphics::Texture2DHandle* guiTextures = nullptr;
		uint32_t guiMeshCount = 0;
		PUG_TRY(gui::EndGUI(guiMeshes, scissorRects, guiTextures, guiMeshCount));
		
		graphics::Mesh* meshes = nullptr;
		graphics::Transform* transforms = nullptr;
		graphics::Material* materials = nullptr;
		graphics::PointLight* pointLights = nullptr;
		graphics::DirectionalLight* directionalLights = nullptr;
		vmath::Vector4* directionalShadowMapBoxes = nullptr;
		uint32_t meshCount = 0;
		uint32_t pointLightCount = 0;
		uint32_t directionalLightCount = 0;
		graphics::Camera camera = {};
		vmath::Vector3 cameraPos = Vector3();
		vmath::Quaternion cameraRotation = Quaternion();
		
		if (g_app != nullptr)
		{
			g_app->GetRenderData(meshes, transforms, materials, meshCount);
			g_app->GetMainCamera(camera, cameraPos, cameraRotation);
			g_app->GetLightData(pointLights, pointLightCount, directionalLights, directionalShadowMapBoxes, directionalLightCount);
		}
		
		vmath::Int2 windowSize = g_window->GetSize();
		vmath::Matrix4 viewMatrix = utility::CreateViewMatrix(cameraPos, cameraRotation);
		vmath::Matrix4 projectionMatrix = utility::CreateProjectionMatrix(camera.fov, (float)windowSize.x / (float)windowSize.y, camera.zNear, camera.zFar);
		
		PUG_TRY(graphics::Render(
			viewMatrix, 
			projectionMatrix, 
			cameraPos, 
			transforms, 
			meshes, 
			meshCount,
			guiMeshes,
			scissorRects,
			guiTextures,
			guiMeshCount));

		PUG_TRY(input::ResolveInput());
	}
}

void pug::assets::DestroyAssetProcessor()
{
	g_app->Destroy();
	delete g_app;

	PUG_TRY(gui::DestroyGUI());
	PUG_TRY(graphics::DestroyGraphics());
	
	delete g_window;

	log::EndLog();
}