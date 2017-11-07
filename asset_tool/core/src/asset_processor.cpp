#include "core/inc/asset_processor.h"

#include "core/inc/macro.h"
#include "graphics/inc/window.h"
#include "graphics/inc/graphics.h"

#include <experimental\filesystem>

using namespace pug;
using namespace pug::platform;
using namespace vmath;

static Window* g_window;

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
		g_window->Update();
	}
}

void pug::assets::DestroyAssetProcessor()
{
	delete g_window;

	log::EndLog();
}