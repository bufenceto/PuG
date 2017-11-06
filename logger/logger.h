#pragma once
#include <string>
#include <sstream>
#include <iostream>

namespace pug{
namespace log{

#define LOG_COLOR_GREEN 0x0A
#define LOG_COLOR_RED 0x0C
#define LOG_COLOR_YELLOW  0x0E
#define LOG_COLOR_WHITE 0x0F

	enum EBreakLevel : uint32_t
	{
		BreakLevel_Info = 15,
		BreakLevel_Message = 14,
		BreakLevel_Warning = 12,
		BreakLevel_Error = 8,
	};

	uint32_t StartLog(const std::string& logFilePath, const pug::log::EBreakLevel breakLevel = EBreakLevel::BreakLevel_Error);
	uint32_t EndLog();

	void SetTextColor(uint16_t color);
	void ResetTextColor();

	void WriteToConsole(const std::string& text);
	void WriteToLog(const std::string& text);

	void Log(const char* log);
	void Info(const char* info);
	void Message(const char* message);
	void Warning(const char* warning);
	void Error(const char* error);

	void LogAssert(const char* error);

	///http://www.stroustrup.com/C++11FAQ.html#variadic-templates
	template<typename T, typename... Args>
	void Log(const char* log, T value, Args... args)
	{
		std::stringstream buffer;
		while (log && *log)
		{
			if (*log == '%' && *++log != '%')
			{	// a format specifier (ignore which one it is)
				buffer << value;//stream in the value
				WriteToLog(buffer.str());
				Log(++log, args...); 	// ``peel off'' first argument
				break;
			}
			buffer << *log++;//stream in characters
		}
	}
	template<typename T, typename... Args>
	void Info(const char* info, T value, Args... args)
	{
		std::stringstream buffer;
		SetTextColor(LOG_COLOR_WHITE);
		while (info && *info)
		{
			if (*info == '%' && *++info != '%')
			{	// a format specifier (ignore which one it is)
				buffer << value;//stream in the value
				WriteToLog(buffer.str());
				WriteToConsole(buffer.str());
				Info(++info, args...); 	// ``peel off'' first argument
				break;
			}
			buffer << *info++;//stream in characters
		}
		ResetTextColor();
	}
	template<typename T, typename... Args>
	void Message(const char* message, T value, Args... args)
	{
		std::stringstream buffer;
		SetTextColor(LOG_COLOR_GREEN);
		while (message && *message)
		{
			if (*message == '%' && *++message != '%')
			{	// a format specifier (ignore which one it is)
				buffer << value;//stream in the value
				WriteToLog(buffer.str());
				WriteToConsole(buffer.str());
				Message(++message, args...); 	// ``peel off'' first argument
				break;
			}
			buffer << *message++;//stream in characters
		}
		ResetTextColor();
	}
	template<typename T, typename... Args>
	void Warning(const char* warning, T value, Args... args)
	{
		std::stringstream buffer;
		SetTextColor(LOG_COLOR_YELLOW);
		while (warning && *warning)
		{
			if (*warning == '%' && *++warning != '%')
			{	// a format specifier (ignore which one it is)
				buffer << value;//stream in the value
				WriteToLog(buffer.str());
				WriteToConsole(buffer.str());
				Warning(++warning, args...); 	// ``peel off'' first argument
				break;
			}
			buffer << *warning++;//stream in characters
		}
		ResetTextColor();
	}
	template<typename T, typename... Args>
	void Error(const char* error, T value, Args... args)
	{
		std::stringstream buffer;
		SetTextColor(LOG_COLOR_RED);
		while (error && *error)
		{
			if (*error == '%' && *++error != '%')
			{	// a format specifier (ignore which one it is)
				buffer << value;//stream in the value
				WriteToLog(buffer.str());
				WriteToConsole(buffer.str());
				Error(++error, args...); 	// ``peel off'' first argument
				break;
			}
			buffer << *error++;//stream in characters
		}
		ResetTextColor();
	}
	template<typename T, typename... Args>
	void LogAssert(const char* error, T value, Args... args)
	{
		std::stringstream buffer;
		SetTextColor(g_colors[1]);
		while (error && *error)
		{
			if (*error == '%' && *++error != '%')
			{	// a format specifier (ignore which one it is)
				buffer << value;//stream in the value
				WriteToLog(buffer.str());
				WriteToConsole(buffer.str());
				Error(++error, args...); 	// ``peel off'' first argument
				break;
			}
			buffer << *error++;//stream in characters
		}
		ResetTextColor();
	}

	std::string GetLogFilePath();
}//pug::log
}//pug