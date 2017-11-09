#pragma once

namespace pug {
namespace time {

	class Timer
	{
	public:
		Timer();
		// Call this once per frame
		double GetFrameDelta();


	private:
		double timerFrequency = 0.0;
		long long lastFrameTime = 0;
		long long lastSecond = 0;
		double frameDelta = 0;
	};

}//vpl::timer
}//vpl