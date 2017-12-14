#pragma once
#include "sha1.h"
#include <experimental/filesystem>

typedef void* HANDLE;

namespace pug{
namespace assets{

	class DirectoryMonitor
	{
	public:
		DirectoryMonitor(const std::experimental::filesystem::path& a_pathToMonitor);
		~DirectoryMonitor();

		DirectoryMonitor(const DirectoryMonitor& other);//copy
		DirectoryMonitor(DirectoryMonitor&& other);//move

		DirectoryMonitor& operator=(const DirectoryMonitor& other);//copy
		DirectoryMonitor& operator=(DirectoryMonitor&& other);//move

		friend bool operator== (const DirectoryMonitor& a_assetMonitor, const SHA1Hash& a_path);

		bool HasFileChanged() const;
		const SHA1Hash& GetFileHash() const;

	private:
		HANDLE m_fileChangeNotificationObject;
		SHA1Hash m_pathHash;
	};

}
}