#include "logger.h"
#include <fstream>
#include <sstream>
#include <experimental\filesystem>
#include <iostream>

#include <windows.h>

#define MAX_PATH_SIZE 260

using namespace std::experimental::filesystem;
using namespace pug::log;

static path g_logFile;
static std::fstream logFileStream;
static CONSOLE_SCREEN_BUFFER_INFO g_csbi;
static uint32_t g_breakLevel;

std::string GetDateAndTimeString()
{
	SYSTEMTIME st;
	GetSystemTime(&st);
	char s[MAX_PATH_SIZE];
	sprintf_s(s, "%d/%d/%d - %d:%d:%d", st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond);
	return s;
}

std::string GetBreakLevelString(EBreakLevel breakLevel)
{
	switch (breakLevel)
	{
	case(EBreakLevel::BreakLevel_Info): return "Info";
	case(EBreakLevel::BreakLevel_Message): return "Message";
	case(EBreakLevel::BreakLevel_Warning): return "Warning";
	case(EBreakLevel::BreakLevel_Error): return "Error";
	default: return "Unknown";
	}
}

void pug::log::SetTextColor(uint16_t color)
{
	HANDLE hstdout = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hstdout, color);
}

void pug::log::ResetTextColor()
{
	HANDLE hstdout = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hstdout, g_csbi.wAttributes);
}

void pug::log::WriteToLog(const std::string& text)
{
	if (!logFileStream.write(text.c_str(), text.length()))
	{
		printf("/nFailed to write to log!/n");
	}
	logFileStream.flush();
}

void pug::log::WriteToConsole(const std::string& text)
{
	printf("%s", text.c_str());
}

uint32_t pug::log::StartLog(const std::string& logFilePath, const pug::log::EBreakLevel breakLevel /* = EBreakLevel::BreakLevel_Error */)
{
	g_logFile = path(logFilePath) / "log.txt";
	logFileStream.open(g_logFile.c_str(), std::fstream::in | std::fstream::out | std::fstream::trunc);

	if (!logFileStream.is_open())
	{
		return -1;
	}

	g_breakLevel = breakLevel;
	std::string breakLevelString = GetBreakLevelString(breakLevel);
	std::string timeAndDate(GetDateAndTimeString());
	WriteToLog(" -- " + timeAndDate + " BEGINNING LOG -- Debug Break Level set to" + breakLevelString + "\n");

	return 0;
}

uint32_t pug::log::EndLog()
{
	std::string timeAndDate = GetDateAndTimeString();
	WriteToLog(" -- " + timeAndDate + " ENDING LOG -- \n");

	logFileStream.close();
	if (logFileStream.is_open())
	{
		Error("Failed to close stream to output file!");
		return -1;
	}

	return 0;
}

void pug::log::Log(const char* log)
{
	//std::stringstream buffer;
	//while (log && *log)
	//{
	//	buffer << *log++;//stream in characters
	//}
	//buffer << ;
	std::string res = std::string("[LOG]: ") + log + '\n';
	WriteToLog(res);
}
void pug::log::Log(const char* log, std::stringstream& buffer)
{
	//while (log && *log)
	//{
	buffer << log;//stream in characters
				  //}
	buffer << std::endl;
	WriteToLog("[LOG]: " + buffer.str());
}

void pug::log::Info(const char* info)
{
	SetTextColor(LOG_COLOR_WHITE);
	std::string res = std::string(info) + '\n';
	WriteToLog(std::string("[INFO]: ") + res);
	WriteToConsole(res);
	ResetTextColor();
#ifdef _DEBUG
	if (g_breakLevel >= BreakLevel_Info)
	{
		__debugbreak();
	}
#endif
}
void pug::log::Info(const char* info, std::stringstream& buffer)
{
	SetTextColor(LOG_COLOR_WHITE);
	buffer << info;//stream in characters
	buffer << std::endl;
	WriteToLog("[INFO]: " + buffer.str());
	WriteToConsole(buffer.str());
	ResetTextColor();
#ifdef _DEBUG
	if (g_breakLevel >= BreakLevel_Info)
	{
		__debugbreak();
	}
#endif
}

void pug::log::Message(const char* message)
{
	SetTextColor(LOG_COLOR_GREEN);
	std::string res = std::string(message) + '\n';
	WriteToLog(std::string("[MESSAGE]: ") + res);
	WriteToConsole(res);
	ResetTextColor();
#ifdef _DEBUG
	if (g_breakLevel >= BreakLevel_Message)
	{
		__debugbreak();
	}
#endif
}
void pug::log::Message(const char* message, std::stringstream& buffer)
{
	SetTextColor(LOG_COLOR_GREEN);
	buffer << message;//stream in characters
	buffer << std::endl;
	WriteToLog("[MESSAGE]: " + buffer.str());
	WriteToConsole(buffer.str());
	ResetTextColor();
#ifdef _DEBUG
	if (g_breakLevel >= BreakLevel_Message)
	{
		__debugbreak();
	}
#endif
}

void pug::log::Warning(const char* warning)
{
	SetTextColor(LOG_COLOR_YELLOW);
	std::string res = std::string(warning) + '\n';
	WriteToLog(std::string("[WARNING]: ") + res);
	WriteToConsole(res);
	ResetTextColor();
#ifdef _DEBUG
	if (g_breakLevel >= BreakLevel_Warning)
	{
		__debugbreak();
	}
#endif
}
void pug::log::Warning(const char* warning, std::stringstream& buffer)
{
	SetTextColor(LOG_COLOR_YELLOW);
	buffer << warning;//stream in characters
	buffer << std::endl;
	WriteToLog("[WARNING]: " + buffer.str());
	WriteToConsole(buffer.str());
	ResetTextColor();
#ifdef _DEBUG
	if (g_breakLevel >= BreakLevel_Warning)
	{
		__debugbreak();
	}
#endif
}

void pug::log::Error(const char* error)
{
	SetTextColor(LOG_COLOR_RED);
	std::string res = std::string(error) + '\n';
	WriteToLog(std::string("[ERROR]: ") + res);
	WriteToConsole(res);
	ResetTextColor();
#ifdef _DEBUG
	if (g_breakLevel >= BreakLevel_Error)
	{
		__debugbreak();
	}
#endif
}

void pug::log::Error(const char* error, std::stringstream& buffer)
{
	SetTextColor(LOG_COLOR_RED);
	buffer << error;//stream in characters
	buffer << std::endl;
	WriteToLog("[ERROR]: " + buffer.str());
	WriteToConsole(buffer.str());
	ResetTextColor();
#ifdef _DEBUG
	if (g_breakLevel >= BreakLevel_Error)
	{
		__debugbreak();
	}
#endif
}

//special case for our assert macro, 
//we dont want to trigger a break point twice
void pug::log::LogAssert(const char* error)
{//just an error msg without breaking
	SetTextColor(LOG_COLOR_RED);
	std::string res = std::string(error) + '\n';
	WriteToLog(std::string("[ASSERT]: ") + res);
	WriteToConsole(res);
	ResetTextColor();
}
void pug::log::LogAssert(const char* error, std::stringstream& buffer)
{
	SetTextColor(LOG_COLOR_RED);
	buffer << error;//stream in characters
	buffer << std::endl;
	WriteToLog("[ASSERT]: " + buffer.str());
	WriteToConsole(buffer.str());
	ResetTextColor();
}

std::string pug::log::GetLogFilePath()
{
	return g_logFile.string();
}