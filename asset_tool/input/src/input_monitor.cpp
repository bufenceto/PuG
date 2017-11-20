#include "input_monitor.h"
#include "input_message.h"
#include "vmath/vmath.h"

#include "input.h"
#include "graphics.h"

#include "logger.h"

#include <memory>
#include <vector>

using namespace pug;
using namespace pug::log;
using namespace pug::input;
using namespace vmath;

#define INPUT_SIZE 256

#define INPUT_UP 0
#define INPUT_DOWN 1
#define INPUT_HELD 2

static uint32_t inputCurrent[INPUT_SIZE * 5];
static uint32_t inputPrevious[INPUT_SIZE * 5];

static Int2 currMousePosition;
static Int2 prevMousePosition;

static int mouseWheelDelta;
static bool mouseVisible;

//window
static uint32_t g_currentWindowMaximized;
static uint32_t g_previousWindowMaximized;

static Int2 g_currentWindowSize;
static Int2 g_previousWindowSize;

struct Event
{
	uint32_t inputID;//the id of the thing that was pressed
	uint32_t eventID;//what happened to the thing that was pressed
	InputCallBack callback;
};

static std::vector<Event> registeredEvents;
static InputCallBack registeredQuitEvent;

PUG_RESULT pug::input::InitializeInput(const uint32_t& windowMaximized, const Int2& windowSize)
{
	g_currentWindowMaximized = windowMaximized;
	g_previousWindowMaximized = windowMaximized;

	g_currentWindowSize = windowSize;
	g_previousWindowSize = windowSize;

	currMousePosition = Int2();
	prevMousePosition = Int2();
	mouseWheelDelta = 0;
	mouseVisible = true;

	memset(inputCurrent, 0, sizeof(uint32_t) * INPUT_SIZE * 5);
	memset(inputPrevious, 0, sizeof(uint32_t) * INPUT_SIZE * 5);

	return PUG_RESULT_OK;
}

PUG_RESULT pug::input::ResolveInput()
{
	for (uint32_t inputID = 0; inputID < PUG_COUNT_OF(inputCurrent); ++inputID)
	{
		//was up and is down -> down event
		if ((inputPrevious[inputID] == INPUT_UP) && (inputCurrent[inputID] == INPUT_DOWN))
		{
			for (uint32_t i = 0; i < registeredEvents.size(); ++i)
			{
				if (registeredEvents[i].inputID == inputID)
				{
					if (registeredEvents[i].eventID == (uint32_t)INPUT_EVENT_PRESSED || registeredEvents[i].eventID == (uint32_t)INPUT_EVENT_HELD)
					{
						registeredEvents[i].callback();
					}
				}
			}
		}
		//was down and is held -> key held event
		if ((inputPrevious[inputID] != INPUT_UP) && ((inputCurrent[inputID] == INPUT_HELD) || (inputCurrent[inputID] == INPUT_DOWN)))
		{
			for (uint32_t i = 0; i < registeredEvents.size(); ++i)
			{
				if (registeredEvents[i].inputID == inputID)
				{
					if (registeredEvents[i].eventID == (uint32_t)INPUT_EVENT_HELD)
					{
						registeredEvents[i].callback();
					}
				}
			}
		}
		//was held or down and is now up -> key up event
		if ((inputPrevious[inputID] != INPUT_UP) && (inputCurrent[inputID] == INPUT_UP))
		{
			for (uint32_t i = 0; i < registeredEvents.size(); ++i)
			{
				if (registeredEvents[i].inputID == inputID)
				{
					if (registeredEvents[i].eventID == (uint32_t)INPUT_EVENT_RELEASED)
					{
						registeredEvents[i].callback();
					}
				}
			}
		}
	}

	if (g_currentWindowSize != g_previousWindowSize)
	{
		Error("Implement me!");
		//graphics::ResizeScreen(g_currentWindowSize);
	}
	if (g_currentWindowMaximized != g_previousWindowMaximized)
	{
		Error("Implement me!");
		//graphics::ResizeScreen(g_currentWindowSize);
	}

	memcpy(inputPrevious, inputCurrent, sizeof(uint32_t) * INPUT_SIZE * 5);
	prevMousePosition = currMousePosition;
	//memset(inputCurrent, 0, sizeof(inputCurrent[0]) * INPUT_SIZE * 5);//clear input;

	g_previousWindowMaximized = g_currentWindowMaximized;
	g_previousWindowSize = g_currentWindowSize;

	return PUG_RESULT_OK;
}

PUG_RESULT pug::input::SubmitInputMessage(const InputMessage& message)
{
	if (message.msg == EMessage::KEY_DOWN)
	{
		inputCurrent[message.data] = INPUT_DOWN;
		if (inputPrevious[message.data])
		{//was down last frame
			inputCurrent[message.data] = INPUT_HELD;
		}
	}
	else if (message.msg == EMessage::KEY_UP)
	{
		inputCurrent[message.data] = INPUT_UP;
	}
	else if (message.msg == EMessage::BUTTON_DOWN)
	{
		if (message.data & (uint32_t)BUTTON_ID_Left)
		{
			inputCurrent[(uint32_t)BUTTON_ID_Left] = INPUT_DOWN;//left
			if (inputPrevious[(uint32_t)BUTTON_ID_Left])
			{
				inputCurrent[(uint32_t)BUTTON_ID_Left] = INPUT_HELD;//left
			}
		}
		if (message.data & (uint32_t)BUTTON_ID_Right)
		{
			inputCurrent[(uint32_t)BUTTON_ID_Right] = INPUT_DOWN;
			if (inputPrevious[(uint32_t)BUTTON_ID_Right])
			{
				inputCurrent[(uint32_t)BUTTON_ID_Right] = INPUT_HELD;//left
			}
		}
		if (message.data & (uint32_t)BUTTON_ID_Middle)
		{
			inputCurrent[(uint32_t)BUTTON_ID_Middle] = INPUT_DOWN;
			if (inputPrevious[(uint32_t)BUTTON_ID_Middle])
			{
				inputCurrent[(uint32_t)BUTTON_ID_Middle] = INPUT_HELD;//left
			}
		}
	}
	else if (message.msg == EMessage::BUTTON_UP)
	{
		if (inputPrevious[(uint32_t)BUTTON_ID_Left] && !(message.data & (uint32_t)BUTTON_ID_Left))
		{
			inputCurrent[(uint32_t)BUTTON_ID_Left] = INPUT_UP;//left
		}
		if (inputPrevious[(uint32_t)BUTTON_ID_Right] && !(message.data & (uint32_t)BUTTON_ID_Right))
		{
			inputCurrent[(uint32_t)BUTTON_ID_Right] = INPUT_UP;
		}
	}
	else if (message.msg == EMessage::MOUSE_MOVE)
	{
		uint32_t x = (uint32_t)(message.data & 0xffff);
		uint32_t y = (uint32_t)((message.data >> 16) & 0xffff);
		currMousePosition = Int2(x, y);
	}
	else if (message.msg == EMessage::QUIT)
	{
		if (registeredQuitEvent != nullptr)
		{
			registeredQuitEvent();
		}
	}
	else if (message.msg == EMessage::RESIZE)
	{//window was resized, store the new width, we will use the last known update dimensions when EXIT_SIZEMOVE happens
		uint32_t shiftCount = sizeof(message.data) << 2;
		uint32_t width = (message.data << shiftCount) >> shiftCount;
		uint32_t height = message.data >> shiftCount;
		g_currentWindowSize = Int2(width, height);

		if (g_currentWindowMaximized)
		{//if resizing happens when we are maximized, we have restored
			g_currentWindowMaximized = 0;
		}
	}
	else if (message.msg == EMessage::MAXIMIZE)
	{
		uint32_t shiftCount = sizeof(message.data) << 2;
		uint32_t width = (message.data << shiftCount) >> shiftCount;
		uint32_t height = message.data >> shiftCount;
		g_currentWindowSize = Int2(width, height);
		g_currentWindowMaximized = 1;
	}
	else if (message.msg == EMessage::EXIT_SIZEMOVE)
	{
		//PUG_TRY(graphics::ResizeScreen(g_reportedWindowSize));
	}

	return PUG_RESULT_OK;
}

PUG_RESULT pug::input::RegisterKeyEvent(const KeyID& key, const InputEvent& event, const InputCallBack& callback)
{
	Event ie =
	{
		(uint32_t)key,
		(uint32_t)event,
		callback,
	};

	registeredEvents.push_back(ie);
	return PUG_RESULT_OK;
}

PUG_RESULT pug::input::RegisterButtonEvent(const ButtonID& button, const InputEvent& event, const InputCallBack& callback)
{
	Event ie =
	{
		(uint32_t)button,
		(uint32_t)event,
		callback,
	};

	registeredEvents.push_back(ie);
	return PUG_RESULT_OK;
}

PUG_RESULT pug::input::RegisterQuitEvent(const InputCallBack& callback)
{
	registeredQuitEvent = callback;
	return PUG_RESULT_OK;
}

//accessors
Int2 pug::input::GetMousePosition()
{
	return currMousePosition;
}

Int2 pug::input::GetMouseDelta()
{
	return currMousePosition - prevMousePosition;
}

bool pug::input::GetKey(KeyID keyID)
{
	return inputCurrent[(uint32_t)keyID] != INPUT_DOWN;
}

bool pug::input::GetButton(ButtonID buttonID)
{
	return inputCurrent[(uint32_t)buttonID] == INPUT_DOWN;
}

const vmath::Int2& pug::input::GetWindowSize()
{
	return g_currentWindowSize;
}