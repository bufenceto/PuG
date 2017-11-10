#include "core/inc/asset_processor.h"

#include "pug_funcs.h"

#include "core/inc/macro.h"
#include "graphics/inc/window.h"
#include "graphics/inc/graphics.h"

#include "time/inc/timer.h"

#include <experimental/filesystem>

using namespace pug;
using namespace pug::platform;
using namespace pug::time;
using namespace vmath;

static Window* g_window;
static Timer g_timer;

static float deltaTime;

float pug::assets::GetDeltaTime()
{
	return deltaTime;
}

void pug::assets::InitAssetProcessor()
{
	log::StartLog(std::experimental::filesystem::current_path().string());

	g_window = Window::Create(Int2(1280, 720), "PuG Asset Processor");
	PUG_ASSERT(g_window, "g_window invalid!");

	PUG_TRY(graphics::InitGraphics(g_window, 0, 0));
}

void pug::assets::RunAssetProcessor()
{
	while (1)
	{
		

		deltaTime = (float)g_timer.GetFrameDelta();
		g_window->Update();

		PUG_TRY(graphics::Render());
	}
}

void pug::assets::DestroyAssetProcessor()
{
	PUG_TRY(graphics::DestroyGraphics());
	delete g_window;

	log::EndLog();
}