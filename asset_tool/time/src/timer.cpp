#include "timer.h"

#include <windows.h>

using namespace pug;
using namespace pug::time;

Timer::Timer()
{
	LARGE_INTEGER li;
	QueryPerformanceFrequency(&li);

	timerFrequency = double(li.QuadPart);

	QueryPerformanceCounter(&li);
	lastFrameTime = li.QuadPart;
}

double Timer::GetFrameDelta()
{
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	frameDelta = double(li.QuadPart - lastFrameTime) / timerFrequency;
	lastFrameTime = li.QuadPart;
	return frameDelta;
}