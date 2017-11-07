#pragma once
#include <cstdint>

namespace pug{
namespace input{

	enum class EMessage : uint32_t
	{
		QUIT = 0,
		ENTER_SIZEMOVE,
		EXIT_SIZEMOVE,
		RESIZE,
		MINIMIZE,
		MAXIMIZE,
		MOUSE_MOVE,
		KEY_DOWN,
		KEY_UP,
		BUTTON_DOWN,//everything besides keyboard keys?
		BUTTON_UP,//everything besides keyboard keys?
		RELOAD_SHADERS,
		UNKNOWN,
	};

	struct InputMessage
	{
		EMessage msg;
		uint32_t data;
	};

}
}