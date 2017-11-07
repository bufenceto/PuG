#include "windows_window.h"

#include "logger.h"

#include "macro.h"
#include "input_monitor.h"
#include "input_message.h"

using namespace pug;
using namespace pug::windows;
using namespace pug::log;
using namespace pug::input;

namespace pug{
namespace windows{

	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	WPARAM MapLeftRightKeys(WPARAM vk, LPARAM lParam);
	EMessage ConvertMessage(UINT msg, WPARAM wParam);

}
}

WindowsWindow::~WindowsWindow()
{
	DestroyWindow(m_windowHandle);
	UnregisterClass(m_title.c_str(), 0);
}

WindowsWindow::WindowsWindow(const WindowsWindow& other)//copy
{

}

WindowsWindow::WindowsWindow(WindowsWindow&& other)//move
{

}

WindowsWindow& WindowsWindow::operator=(const WindowsWindow& other)//copy
{
	return *this;
}

WindowsWindow& WindowsWindow::operator=(WindowsWindow&& other)//move
{
	return *this;
}

PUG_RESULT WindowsWindow::Update()
{
	// Process any messages in the queue.
	MSG msg;
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		//while (GetMessage(&msg, m_windowHandle, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);

	}
	return PUG_RESULT_OK;
}

PUG_RESULT WindowsWindow::Init(const vmath::Int2& a_size, const std::string& a_title)
{
	m_size = a_size;
	m_title = a_title;

	WNDCLASS wc;
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;// | CS_NOCLOSE;
	wc.lpfnWndProc = (WNDPROC)WindowProc;
	wc.cbClsExtra = 0;                           // No extra class data
	wc.cbWndExtra = sizeof(void*) + sizeof(int); // Make room for one pointer
	wc.hInstance = GetModuleHandle(NULL);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = NULL;                        // No background
	wc.lpszMenuName = NULL;                        // No menu
	wc.lpszClassName = a_title.c_str();

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
		return PUG_RESULT_PLATFORM_ERROR;
	}

	// Calculate full window size (including borders)
	DWORD m_style = WS_OVERLAPPEDWINDOW;

	RECT rect = { 0, 0, a_size.x, a_size.y };
	AdjustWindowRect(&rect, m_style, FALSE);
	int width = rect.right - rect.left;
	int height = rect.bottom - rect.top;

	m_windowHandle = CreateWindow(
		a_title.c_str(),
		a_title.c_str(),
		m_style,
		CW_USEDEFAULT, CW_USEDEFAULT,
		width, height,
		0, 0,
		GetModuleHandle(0),
		0);

	if (m_windowHandle == NULL)
	{
		return PUG_RESULT_PLATFORM_ERROR;
	}

	ShowWindow(m_windowHandle, SW_SHOWNORMAL);

	return PUG_RESULT_OK;
}

void SetTitle(const std::string& a_title)
{
	Error("Plz implement");
}

// Main message handler for the sample.
LRESULT CALLBACK pug::windows::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	pug::input::InputMessage pugMsg = {};
	pugMsg.msg = ConvertMessage(message, wParam);

	if (pugMsg.msg == EMessage::UNKNOWN)
	{
		// Handle any untranslated msgs
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	else if (pugMsg.msg == EMessage::KEY_DOWN)
	{
		pugMsg.data = (uint32_t)MapLeftRightKeys(wParam, lParam);
	}
	else if (pugMsg.msg == EMessage::KEY_UP)
	{
		pugMsg.data = (uint32_t)MapLeftRightKeys(wParam, lParam);
	}
	else if (pugMsg.msg == EMessage::BUTTON_DOWN)
	{
		pugMsg.data = (uint32_t)wParam;
	}
	else if (pugMsg.msg == EMessage::BUTTON_UP)
	{
		pugMsg.data = (uint32_t)wParam;
	}
	else if (pugMsg.msg == EMessage::MOUSE_MOVE)
	{
		pugMsg.data = (uint32_t)lParam;
	}
	else if (pugMsg.msg == EMessage::RESIZE)
	{//also called on size restore
		pugMsg.data = (uint32_t)lParam;
	}
	else if (pugMsg.msg == EMessage::MAXIMIZE)
	{
		pugMsg.data = (uint32_t)lParam;
	}
	else if (pugMsg.msg == EMessage::EXIT_SIZEMOVE)
	{
	}
	else if (pugMsg.msg == EMessage::QUIT)
	{

	}
	else
	{
		log::Info("Input message was translated but not handled!");
	}

	PUG_TRY(SubmitInputMessage(pugMsg));

	return 0;
}

WPARAM pug::windows::MapLeftRightKeys(WPARAM vk, LPARAM lParam)
{
	WPARAM new_vk = vk;
	UINT scancode = (lParam & 0x00ff0000) >> 16;
	int extended = (lParam & 0x01000000) != 0;

	switch (vk)
	{
	case VK_SHIFT:
	{
		new_vk = MapVirtualKey(scancode, MAPVK_VSC_TO_VK_EX);
		break;
	}
	case VK_CONTROL:
	{
		new_vk = extended ? VK_RCONTROL : VK_LCONTROL;
		break;
	}
	case VK_MENU:
	{
		new_vk = extended ? VK_RMENU : VK_LMENU;
		break;
	}
	default:
	{
		new_vk = vk;
		break;
	}
	}

	return new_vk;
}

EMessage pug::windows::ConvertMessage(UINT msg, WPARAM wParam)
{
	switch (msg)
	{
	case WM_CLOSE:
	case WM_QUIT:
	case WM_DESTROY:
	case WM_NCDESTROY:
		return EMessage::QUIT;
	case WM_SIZE:
		if (wParam == SIZE_MAXIMIZED)
		{
			return EMessage::MAXIMIZE;
		}
		else if (wParam == SIZE_MINIMIZED)
		{
			return EMessage::MINIMIZE;
		}
		else
		{
			return EMessage::RESIZE;
		}
	case WM_EXITSIZEMOVE:
		return EMessage::EXIT_SIZEMOVE;
	case WM_ENTERSIZEMOVE:
		return EMessage::ENTER_SIZEMOVE;
	case WM_MOUSEMOVE:
		return EMessage::MOUSE_MOVE;
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
		return EMessage::BUTTON_DOWN;
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MBUTTONUP:
		return EMessage::BUTTON_UP;
	case WM_KEYDOWN:
		return EMessage::KEY_DOWN;
	case WM_KEYUP:
		return EMessage::KEY_UP;
	default:
		return EMessage::UNKNOWN;
	}
}

using namespace platform;

Window* Window::Create(const vmath::Int2& a_size, const std::string& a_title)
{
	WindowsWindow* window = new WindowsWindow();
	window->Init(a_size, a_title);

	return window;
}