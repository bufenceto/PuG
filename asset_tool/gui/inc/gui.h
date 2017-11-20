#pragma once
#include <cstdint>
#include "macro.h"
#include "result_codes.h"

#include "handles.h"

namespace pug{
namespace platform{
	class Window;
}//pug::platform
namespace assets{
namespace graphics{
	struct Mesh;
	struct Rect;
}//pug::assetsgraphics

namespace gui{

	PUG_RESULT InitGUI(pug::platform::Window* window);
	PUG_RESULT DestroyGUI();

	PUG_RESULT StartGUI(
		float dt, 
		pug::platform::Window* window);
	PUG_RESULT EndGUI(
		pug::assets::graphics::Mesh*& out_meshes,
		pug::assets::graphics::Rect*& out_scissorRects,
		pug::assets::graphics::Texture2DHandle*& out_textures,
		uint32_t& out_meshCount);

	bool GUIHovered();
	//get gui vertex buffers and index buffers

}//vpl::assets::gui
}//pug::assets
}//pug
