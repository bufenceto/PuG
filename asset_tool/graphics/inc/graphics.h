#pragma once
#include "core/inc/result_codes.h"
#include "vmath.h"

namespace pug {
namespace platform{
	class Window;
}
namespace assets{
namespace graphics{

	PUG_RESULT InitGraphics(
		pug::platform::Window* a_window,
		uint32_t verticalSyncInterval,
		uint32_t fullscreen);
	PUG_RESULT DestroyGraphics();

	PUG_RESULT Render();

	PUG_RESULT CreateTexture2D();
}
}
}