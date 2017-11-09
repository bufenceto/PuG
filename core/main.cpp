
#include "logger.h"
#include "window.h"
#include "dx12_renderer.h"
#include "vmath\vmath.h"

using namespace pug;
using namespace pug::graphics;

int main()
{
	log::StartLog("x64/Debug", log::BreakLevel_Warning);

	Window* window = Window::Create("MY_WINDOW_NAME", vmath::Int2(800, 640));
	if (!window)
	{
		log::Error("Error initializing window.");
		log::EndLog();

		return 0;
	}

	DX12Renderer* renderer = new DX12Renderer();
	if (!PUG_SUCCEEDED(renderer->Initialize(window)))
	{
		log::Error("Error initializing the renderer.");
		log::EndLog();

		renderer->Destroy();
		window->Destroy();

		return 0;
	}

	// Main loop
	while (true)
	{
		window->DispatchMessages();
		renderer->Draw();
	}

	renderer->Destroy();
	window->Destroy();
	return 0;
}
