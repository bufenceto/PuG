#include "win32_window.h"

namespace pug {
namespace graphics {

	LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	Window* Window::Create(const std::string& a_title, const vmath::Int2& a_size)
	{
		Win32Window* window = new Win32Window();
		window->Initialize(a_title, a_size);

		return window;
	}

	RESULT Win32Window::Initialize(const std::string& a_title, const vmath::Int2& a_size)
	{
		m_size = a_size;

		WNDCLASS wc;
		wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;// | CS_NOCLOSE;
		wc.lpfnWndProc = (WNDPROC)WndProc;
		wc.cbClsExtra = 0;                           // No extra class data
		wc.cbWndExtra = sizeof(void*) + sizeof(int); // Make room for one pointer
		wc.hInstance = GetModuleHandle(NULL);
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = NULL;                        // No background
		wc.lpszMenuName = NULL;                        // No menu
		wc.lpszClassName = "DirectXWindowClass";

		// Load user-provided icon if available
		wc.hIcon = LoadIcon(GetModuleHandle(NULL), "GLFW_ICON");
		if (!wc.hIcon)
		{
			// No user-provided icon found, load default icon
			wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
		}

		if (!RegisterClass(&wc))
		{
			// Failed to register window class
			return RESULT_FAILED;
		}

		// Calculate full window size (including borders)
		DWORD m_style = WS_OVERLAPPEDWINDOW;

		RECT rect = { 0, 0, a_size.x, a_size.y };
		AdjustWindowRect(&rect, m_style, FALSE);
		uint32_t width = rect.right - rect.left;
		uint32_t height = rect.bottom - rect.top;
		int x = (GetSystemMetrics(SM_CXSCREEN) - rect.right) / 2 + rect.left;
		int y = (GetSystemMetrics(SM_CYSCREEN) - rect.bottom) / 2 + rect.top;
		m_hwnd = CreateWindow(
			wc.lpszClassName,
			a_title.c_str(),
			m_style,
			x, y,
			width, height,
			0, 0,
			GetModuleHandle(0),
			nullptr);

		if (m_hwnd == NULL)
		{
			return RESULT_FAILED;
		}

		ShowWindow(m_hwnd, 1);

		return RESULT_OK;
	}

	void Win32Window::Destroy()
	{

	}

	void Win32Window::DispatchMessages()
	{
		MSG msg;
		while (PeekMessage(&msg, m_hwnd, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

	}

	void Win32Window::SetTitle(const std::string& a_title)
	{
		SetWindowText(m_hwnd, a_title.c_str());
	}

	LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg)
		{
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;

		}

		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
}
}