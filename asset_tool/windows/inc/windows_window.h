#pragma once
#include "window.h"
#include "result_codes.h"

#include <Windows.h>

namespace pug {
	namespace windows {

		class WindowsWindow : public pug::platform::Window
		{
		public:
			WindowsWindow() = default;
			~WindowsWindow();

			WindowsWindow(const WindowsWindow& other);//copy
			WindowsWindow(WindowsWindow&& other);//move

			WindowsWindow& operator=(const WindowsWindow& other);//copy
			WindowsWindow& operator=(WindowsWindow&& other);//move

			PUG_RESULT Update() override;

			PUG_RESULT Init(const vmath::Int2& a_size, const std::string& a_title);

			const std::string GetTitle() const { return m_title; }
			const HWND GetWindowHandle() const { return m_windowHandle; }
			
			void SetTitle(const std::string& a_title);

		private:
			std::string m_title;
			DWORD m_style;
			HWND m_windowHandle;
		};

	}
}
