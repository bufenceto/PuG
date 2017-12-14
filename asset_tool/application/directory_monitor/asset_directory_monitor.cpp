#include "asset_directory_monitor.h"

//pug system headers
#include "logger.h"

//project headers
#include "sha1.h"
#include "asset_types.h"
#include "importers/mesh_converter.h"
#include "importers/texture_converter.h"

//pug headers
#include "macro.h"

//external
#include <vector>
#include <Windows.h>
#include <process.h>
#include <experimental/filesystem>

using namespace std;
using namespace std::experimental::filesystem;
using namespace pug;
using namespace pug::assets;

#define NUM_THREADS 2

static path g_monitoredAssetBasePath;
static vector<path> g_monitoredFiles;
static HANDLE g_cookThreadHandles[NUM_THREADS];
static HANDLE g_monitorThreadHandle;

ReportDirectoryChangeCallBack g_reportChangeCallBack;
static uint32_t g_threadsRunning = 1;

static uint32_t WINAPI DirectoryMonitorThreadMain(void*);

PUG_FILE_ACTION TranslateFileAction(DWORD windowsFileAction)
{
	switch (windowsFileAction)
	{
	case FILE_ACTION_ADDED:	return PUG_FILE_ACTION_ADDED; break;
	case FILE_ACTION_REMOVED: return PUG_FILE_ACTION_REMOVED; break;
	case FILE_ACTION_MODIFIED: return PUG_FILE_ACTION_MODIFIED; break;
	case FILE_ACTION_RENAMED_OLD_NAME: return PUG_FILE_ACTION_RENAMED_OLD_NAME; break;
	case FILE_ACTION_RENAMED_NEW_NAME: return PUG_FILE_ACTION_RENAMED_NEW_NAME; break;
	default:
		log::Warning("Unrecognized file action");
		return PUG_FILE_ACTION_UNKNOWN;
	}
}

static std::string ws2s(const std::wstring& wstr)
{
	using convert_type = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_type, wchar_t> converter;

	return converter.to_bytes(wstr);
}

int32_t FindFileMonitor(const std::experimental::filesystem::path& a_relativeAssetPath)
{
	int32_t fileMonitorIndex = -1;
	for (int32_t i = 0; i < (int32_t)g_monitoredFiles.size(); ++i)
	{
		if (g_monitoredFiles[i] == a_relativeAssetPath)
		{
			fileMonitorIndex = i;
			break;
		}
	}
	return fileMonitorIndex;
}

PUG_RESULT pug::assets::InitAssetDirectoryMonitor(
	const std::experimental::filesystem::path& a_absoluteAssetBasePath,
	ReportDirectoryChangeCallBack a_reportChangeCallBack)
{
	if (a_reportChangeCallBack == nullptr)
	{
		return PUG_RESULT_INVALID_ARGUMENTS;
	}

	g_monitoredAssetBasePath = a_absoluteAssetBasePath;
	g_reportChangeCallBack = a_reportChangeCallBack;

	uint32_t threadID = 0;
	g_monitorThreadHandle = (HANDLE)_beginthreadex(nullptr, 0, DirectoryMonitorThreadMain, nullptr, 0, &threadID);
	if (g_monitorThreadHandle == nullptr)
	{
		log::Error("Failed to open directory monitor thread handle");
		return PUG_RESULT_FAILED_TO_CREATE_OS_OBJECT;
	}

	return PUG_RESULT_OK;
}

PUG_RESULT pug::assets::DestroyAssetDirectoryMonitor()
{
	g_threadsRunning = 0;
	TerminateThread(g_monitorThreadHandle, 0);

	return PUG_RESULT_OK;
}

static uint32_t WINAPI DirectoryMonitorThreadMain(void*)
{
	do
	{
		HANDLE directoryHandle =
			CreateFile(
				g_monitoredAssetBasePath.string().c_str(),
				GENERIC_READ,
				FILE_SHARE_READ | FILE_SHARE_WRITE,
				0,
				OPEN_EXISTING,
				FILE_FLAG_BACKUP_SEMANTICS,
				nullptr);

		DWORD* changeBuffer[512] = {};
		DWORD bytesReturned = 0;
		ReadDirectoryChangesW(
			directoryHandle,
			changeBuffer,
			sizeof(changeBuffer),
			TRUE,
			FILE_NOTIFY_CHANGE_LAST_WRITE,
			&bytesReturned,
			NULL,
			nullptr);

		PUG_ASSERT(sizeof(changeBuffer) >= bytesReturned, "buffer was not big enough!");

		_FILE_NOTIFY_INFORMATION* curr = (_FILE_NOTIFY_INFORMATION*)changeBuffer;
		do
		{
			DirectoryChange pugDirChange = {};

			std::string fileName = ws2s(curr->FileName);
			//copy file name to report struct
			memcpy(&pugDirChange.fileName, fileName.c_str(), fileName.length());
			pugDirChange.fileAction = TranslateFileAction(curr->Action);

			g_reportChangeCallBack(pugDirChange);

			curr += curr->NextEntryOffset;
		}
		while(curr->NextEntryOffset != 0);

	} 
	while (g_threadsRunning);

	//}
	_endthreadex(0);
	return 0;
}