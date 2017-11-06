#include "logger.h"
#include <fstream>
#include <sstream>
#include <experimental\filesystem>
#include <iostream>

#include <windows.h>

#define MAX_PATH_SIZE 260

using namespace std::experimental::filesystem;
using namespace vpl::log;

static path g_logFile;
static std::fstream logFileStream;
static CONSOLE_SCREEN_BUFFER_INFO g_csbi;
static uint32_t g_breakLevel;

void GetDateAndTimeString(std::string& out_string)
{
	SYSTEMTIME st;
	GetSystemTime(&st);
	char s[MAX_PATH_SIZE];
	sprintf_s(s, "%d/%d/%d - %d:%d:%d", st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond);
	out_string = s;
}

void GetBreakLevelString(EBreakLevel breakLevel, std::string& out_string)
{
	switch (breakLevel)
	{
	case(EBreakLevel::BreakLevel_Info): out_string = "Info";
	case(EBreakLevel::BreakLevel_Message): out_string = "Message";
	case(EBreakLevel::BreakLevel_Warning): out_string = "Warning";
	case(EBreakLevel::BreakLevel_Error): out_string = "Error";
	default: out_string = "Unknown";
	}
}

void vpl::log::SetTextColor(uint16_t color)
{
	HANDLE hstdout = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hstdout, color);
}

void vpl::log::ResetTextColor()
{
	HANDLE hstdout = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hstdout, g_csbi.wAttributes);
}

void vpl::log::WriteToLog(const std::string& text)
{
	if (!logFileStream.write(text.c_str(), text.length()))
	{
		printf("/nFailed to write to log!/n");
	}
	logFileStream.flush();
}

void vpl::log::WriteToConsole(const std::string& text)
{
	printf("%s", text.c_str());
}

uint32_t vpl::log::StartLog(const std::string& logFilePath, const vpl::log::EBreakLevel breakLevel /* = EBreakLevel::BreakLevel_Error */)
{
	g_logFile = path(logFilePath) / "log.txt";
	logFileStream.open(g_logFile.c_str(), std::fstream::in | std::fstream::out | std::fstream::trunc);
	
	if (!logFileStream.is_open())
	{
		return -1;
	}

	g_breakLevel = breakLevel;
	std::string breakLevelString;
	GetBreakLevelString(breakLevel, breakLevelString);
	std::string timeAndDate;
	GetDateAndTimeString(timeAndDate);
	WriteToLog(" -- " + timeAndDate + " BEGINNING LOG -- Debug Break Level set to" + breakLevelString + "\n");

	return 0;
}

uint32_t vpl::log::EndLog()
{
	std::string timeAndDate;
	GetDateAndTimeString(timeAndDate);
	WriteToLog(" -- " + timeAndDate + " ENDING LOG -- \n");

	logFileStream.close();
	if (logFileStream.is_open())
	{
		Error("Failed to close stream to output file!");
		return -1;
	}

	return 0;
}

void vpl::log::Log(const char* log)
{
	std::stringstream buffer;
	while (log && *log)
	{
		buffer << *log++;//stream in characters
	}
	buffer << std::endl;
	WriteToLog(buffer.str());
}
void vpl::log::Info(const char* info)
{
	std::stringstream buffer;
	SetTextColor(LOG_COLOR_WHITE);
	while (info && *info)
	{
		buffer << *info++;//stream in characters
	}
	buffer << std::endl;
	WriteToLog(buffer.str());
	WriteToConsole(buffer.str());
	ResetTextColor();
	if (g_breakLevel >= BreakLevel_Info)
	{
		__debugbreak();
	}
}
void vpl::log::Message(const char* message)
{
	std::stringstream buffer;
	SetTextColor(LOG_COLOR_GREEN);
	while (message && *message)
	{
		buffer << *message++;//stream in characters
	}
	buffer << std::endl;
	WriteToLog(buffer.str());
	WriteToConsole(buffer.str());
	ResetTextColor();
	if (g_breakLevel >= BreakLevel_Message)
	{
		__debugbreak();
	}
}
void vpl::log::Warning(const char* warning)
{
	std::stringstream buffer;
	SetTextColor(LOG_COLOR_YELLOW);
	while (warning && *warning)
	{
		buffer << *warning++;//stream in characters
	}
	buffer << std::endl;
	WriteToLog(buffer.str());
	WriteToConsole(buffer.str());
	ResetTextColor();
	if (g_breakLevel >= BreakLevel_Warning)
	{
		__debugbreak();
	}
}
void vpl::log::Error(const char* error)
{
	std::stringstream buffer;
	SetTextColor(LOG_COLOR_RED);
	while (error && *error)
	{
		buffer << *error++;//stream in characters
	}
	buffer << std::endl;
	WriteToLog(buffer.str());
	WriteToConsole(buffer.str());
	ResetTextColor();
	if (g_breakLevel >= BreakLevel_Error)
	{
		__debugbreak();
	}
}
//special case for our assert macro, 
//we dont want to trigger a break point twice
void vpl::log::LogAssert(const char* error)
{//just an error msg without breaking
	std::stringstream buffer;
	SetTextColor(LOG_COLOR_RED);
	while (error && *error)
	{
		buffer << *error++;//stream in characters
	}
	buffer << std::endl;
	WriteToLog(buffer.str());
	WriteToConsole(buffer.str());
	ResetTextColor();
}

std::string vpl::log::GetLogFilePath()
{
	return g_logFile.string();
}