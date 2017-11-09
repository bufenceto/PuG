#pragma once
#include <Windows.h>
#include "window.h"

namespace pug {
namespace graphics {

	class Win32Window : public Window
	{
	public:
		Win32Window() = default;
		~Win32Window() {}

		virtual void Destroy() final;
		virtual void DispatchMessages() final;

		void SetTitle(const std::string& a_title);
		HWND GetWindowHandle()
		{
			return m_hwnd;
		}

		PUG_RESULT Initialize(const std::string& a_title, const vmath::Int2& a_size);

	private:
		HWND m_hwnd;
	};

}
}