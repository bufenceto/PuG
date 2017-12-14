#include "directory_monitor.h"

#include "macro.h"

#include <Windows.h>

using namespace pug;
using namespace pug::assets;
using namespace std::experimental::filesystem;

DirectoryMonitor::DirectoryMonitor(const path& a_pathToMonitor)
	: m_pathHash(SHA1Hash(a_pathToMonitor.string().c_str()))
{
	path canonicalPathToMonitor = canonical(a_pathToMonitor);
	m_fileChangeNotificationObject = FindFirstChangeNotificationA(canonicalPathToMonitor.string().c_str(), TRUE, FILE_NOTIFY_CHANGE_LAST_WRITE);
}

DirectoryMonitor::~DirectoryMonitor()
{
	PUG_ASSERT(CloseHandle(m_fileChangeNotificationObject), "Failed to close change notification object handle");
}

DirectoryMonitor::DirectoryMonitor(const DirectoryMonitor& other)
{
	m_fileChangeNotificationObject = other.m_fileChangeNotificationObject;
	m_pathHash = other.m_pathHash;
}

DirectoryMonitor::DirectoryMonitor(DirectoryMonitor&& other)
{
	m_fileChangeNotificationObject = other.m_fileChangeNotificationObject;
	m_pathHash = std::move(other.m_pathHash);
}

DirectoryMonitor& DirectoryMonitor::operator=(const DirectoryMonitor& other)
{
	m_fileChangeNotificationObject = other.m_fileChangeNotificationObject;
	m_pathHash = other.m_pathHash;
	return *this;
}

DirectoryMonitor& DirectoryMonitor::operator=(DirectoryMonitor&& other)
{
	m_fileChangeNotificationObject = other.m_fileChangeNotificationObject;
	m_pathHash = std::move(other.m_pathHash);
	return *this;
}

bool DirectoryMonitor::HasFileChanged() const
{
	DWORD res = WaitForSingleObject(m_fileChangeNotificationObject, 0);
	return res == WAIT_OBJECT_0;
}

const SHA1Hash& DirectoryMonitor::GetFileHash() const
{
	return m_pathHash;
}


bool pug::assets::operator== (
	const DirectoryMonitor& a_directoryMonitor,
	const SHA1Hash& a_pathHash)
{
	return a_directoryMonitor.m_pathHash == a_pathHash;
}