#pragma once
#include <cstdint>
#include "result_codes.h"


namespace pug {
namespace graphics {
	class Window;

	class IRenderer
	{
	public:
		virtual RESULT Initialize(Window* a_window) = 0;
		virtual RESULT Resize(Window* a_window) = 0;
		virtual void Draw(/*some arguments here probably*/) = 0;
		virtual void Destroy() = 0;
	};

}
}