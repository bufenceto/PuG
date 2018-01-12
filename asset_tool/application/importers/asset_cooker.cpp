#include "asset_cooker.h"

#include "texture_converter.h"
#include "mesh_converter.h"
#include "sha1.h"
#include "cook_job.h"

#include "macro.h"

#include <vector>
#include <Windows.h>
#include <process.h>
#include <fstream>

#define LIBRARY_FILE_NAME "pug_asset_library.pal"
#define NUM_WORKER_THREADS 2

using namespace std;
using namespace std::experimental::filesystem;
using namespace pug;
using namespace pug::assets;
using namespace pug::log;

struct AssetEntry
{
	SHA1Hash m_assetHash;
	AssetType m_type;
	char m_extension[8];
};
static_assert(sizeof(AssetEntry) == 32, "Invalid Asset Entry Size");

HANDLE g_workerThreads[NUM_WORKER_THREADS];
HANDLE g_workerThreadSemaphore;

HANDLE g_cleanUpThread;
HANDLE g_cleanUpThreadSemaphore;

CRITICAL_SECTION g_cookJobsGuard;
CRITICAL_SECTION g_activeJobsGuard;
CRITICAL_SECTION g_outputFileOperationsGuard;
CRITICAL_SECTION g_deleteGuard;

static vector<CookJob> g_queuedCookJobs;
static vector<CookJob> g_activeJobs;

static vector<path> g_deletedAssets;

static path g_assetOutputPath;
static path g_assetBasePath;

static vector<std::pair<AssetEntry, AssetSettings>> g_assetEntries;//we will output these to the mal file, to be used in the engine for loading files

static AssetConverter* g_converters[] =
{
	new MeshConverter(),
	new TextureConverter(),
};

static uint32_t g_threadsRunning;

static uint32_t WINAPI CookThreadMain(void*);
static uint32_t WINAPI CleanUpThreadMain(void*);

void WriteAssetEntriesToFile()
{
	EnterCriticalSection(&g_outputFileOperationsGuard);

	fstream assetLibraryFile;

	path libraryFilePath = g_assetOutputPath / LIBRARY_FILE_NAME;
	assetLibraryFile.open(libraryFilePath, fstream::out | fstream::binary | fstream::trunc);
	if (assetLibraryFile.is_open())
	{
		//write file header
		uint32_t assetEntryAndSettingsSize = sizeof(AssetEntry) + sizeof(AssetSettings);
		uint32_t numAssetEntries = (uint32_t)g_assetEntries.size();
		if ((!assetLibraryFile.write((char*)&numAssetEntries, sizeof(numAssetEntries))))
		{
			Error("Failed to write asset count to library file!");
		}
		if ((!assetLibraryFile.write((char*)&assetEntryAndSettingsSize, sizeof(assetEntryAndSettingsSize))))
		{
			Error("Failed to write asset entry size to library file!");
		}
		//write file body
		char* data = nullptr;
		if (g_assetEntries.size() > 0)
		{
			data = (char*)&g_assetEntries[0];
		}
		if (!assetLibraryFile.write(data, assetEntryAndSettingsSize * numAssetEntries))
		{
			Error("Failed to write asset entries to library file!");
		}
	}
	else
	{
		Error("Failed to write to library file, file not open!");
	}
	//flush changes to file
	assetLibraryFile.close();
	LeaveCriticalSection(&g_outputFileOperationsGuard);
}

PUG_RESULT ReadAssetEntriesFromFile()
{
	//look for library folder in "executable folder/../library"
	path libraryDir = g_assetOutputPath;

	uint32_t numEntries = 0;
	uint32_t entrySize = 0;
	//import library from file
	//guarantee the file exists
	if (!exists(libraryDir))
	{//directory does not exist
		if (!create_directories(libraryDir))
		{
			Error("Failed to create library directory");
			return PUG_RESULT_FAILED_TO_CREATE_FILE_OR_DIR;
		}
	}

	path libraryFilePath = libraryDir / "pug_asset_library.pal";
	if (!exists(libraryFilePath))
	{//file does not exist
		HANDLE h = CreateFile(
			libraryFilePath.string().c_str(),
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			0,
			CREATE_NEW,
			FILE_ATTRIBUTE_NORMAL,
			nullptr);
		CloseHandle(h);
	}

	if (file_size(libraryFilePath) > 0)
	{//there is data in the file
		fstream libraryFileStream;
		libraryFileStream.open(libraryFilePath.string().c_str(), fstream::in | fstream::binary);
		if (!libraryFileStream.is_open())
		{
			Error("Failed to open library file!");
			return PUG_RESULT_FAILED_TO_READ_FILE;
		}
		libraryFileStream.read((char*)&numEntries, sizeof(uint32_t));
		if (libraryFileStream.fail())
		{
			Error("Failed to read library file header for entry count!\n");
			return PUG_RESULT_FAILED_TO_READ_FILE;
		}
		if (!libraryFileStream.read((char*)&entrySize, sizeof(uint32_t)))
		{
			Error("Failed to read library file header for entry size!\n");
			return PUG_RESULT_FAILED_TO_READ_FILE;
		}
		//PUG_ASSERT(numEntries < MAX_ASSETS, "The number of entries in the mal file exceeds our maximum allowed asset count!");
		for (uint32_t i = 0; i < numEntries; ++i)
		{
			std::pair<AssetEntry, AssetSettings> entry;
			if (!libraryFileStream.read((char*)&entry, entrySize))//read 1 entry
			{
				Error("Failed to read asset entries from library file !\n");
			}
			g_assetEntries.push_back(entry);
		}
	}

	/*
	if (exists(libraryDir))
	{
		path libraryFilePath = libraryDir / "pug_asset_library.pal";
		if (!exists(libraryFilePath))
		{
			HANDLE h = CreateFile(
				libraryFilePath.string().c_str(), 
				GENERIC_READ | GENERIC_WRITE,
				FILE_SHARE_READ | FILE_SHARE_WRITE,
				0,
				CREATE_NEW,
				FILE_ATTRIBUTE_NORMAL,
				nullptr);
			CloseHandle(h);
		}

		fstream libraryFileStream;
		libraryFileStream.open(libraryFilePath.string().c_str(), fstream::in | fstream::binary);
		if (!libraryFileStream.is_open())
		{
			Error("Failed to open library file!");
			return PUG_RESULT_FAILED_TO_READ_FILE;
		}
		libraryFileStream.read((char*)&numEntries, sizeof(uint32_t));
		if (libraryFileStream.fail())
		{
			Error("Failed to read library file header for entry count!\n");
			return PUG_RESULT_FAILED_TO_READ_FILE;
		}
		if (!libraryFileStream.read((char*)&entrySize, sizeof(uint32_t)))
		{
			Error("Failed to read library file header for entry size!\n");
			return PUG_RESULT_FAILED_TO_READ_FILE;
		}
		//PUG_ASSERT(numEntries < MAX_ASSETS, "The number of entries in the mal file exceeds our maximum allowed asset count!");
		for (uint32_t i = 0; i < numEntries; ++i)
		{
			std::pair<AssetEntry, AssetSettings> entry;
			if (!libraryFileStream.read((char*)&entry, entrySize))//read 1 entry
			{
				Error("Failed to read asset entries from library file !\n");
			}
			g_assetEntries.push_back(entry);
		}
	}
	else
	{
		Error("directory %s does not exist!", libraryDir.string().c_str());
		return PUG_RESULT_FILE_DOES_NOT_EXIST;
	}
	*/

	Info("Finished importing asset library from directory %s", libraryDir.string().c_str());
	return PUG_RESULT_OK;
}

int32_t FindAssetEntry(const SHA1Hash& a_assetHash)
{
	int32_t index = -1;
	for (int32_t i = 0; i < (int32_t)g_assetEntries.size(); ++i)
	{
		if (g_assetEntries[i].first.m_assetHash == a_assetHash)
		{
			index = i;
			break;
		}
	}
	return index;
}

void FormatAndUpdateAssetEntry(
	const path& a_relativeAssetPath,
	const AssetType& a_type,
	const char* a_extension,
	const AssetSettings& a_assetSettings)
{
	SHA1Hash assetHash = a_relativeAssetPath.string().c_str();;
	//char assetEntry[ASSET_ENTRY_SIZE];
	AssetEntry assetEntry;
	//utility::SHA1(relativeAssetPath.string(), assetEntry, SHA1_HASH_BYTES);//the hash function will write the result to the first 20 bytes after the passed ptr
	assetEntry.m_assetHash = assetHash;
	//memcpy(assetEntry + SHA1_HASH_BYTES, &type, sizeof(uint32_t));
	assetEntry.m_type = a_type;
	//memcpy(assetEntry + SHA1_HASH_BYTES + sizeof(uint32_t), extension, ASSET_ENTRY_SIZE - (SHA1_HASH_BYTES + sizeof(uint32_t)));
	memcpy(assetEntry.m_extension, a_extension, sizeof(assetEntry.m_extension));
	
	//add or update formated asset entry to asset buffer
	int32_t assetEntryIndex = FindAssetEntry(assetHash);
	if (assetEntryIndex != -1)
	{
		g_assetEntries[assetEntryIndex] = std::pair<AssetEntry, AssetSettings>(assetEntry, a_assetSettings);
	}
	else
	{
		g_assetEntries.push_back(std::pair<AssetEntry, AssetSettings>(assetEntry, a_assetSettings));
	}
}

void CookAsset(
	const path& a_absoluteRawAssetPath,
	const path& a_outputDirectoryPath,
	const path& a_relativeAssetPath,
	const AssetSettings& a_assetSettings,
	const uint32_t a_forceCook)
{
	AssetConverter* suitableConverter = nullptr;

	for (uint32_t i = 0; i < sizeof(g_converters) / sizeof(g_converters[0]); ++i)
	{
		path extension = a_absoluteRawAssetPath.extension();
		if (extension.string().length() > 8)
		{
			Error("Files that have an extension of more than 8 characters are not supported");
		}
		if (g_converters[i]->IsExtensionSupported(extension))
		{
			suitableConverter = g_converters[i];
			break;
		}
	}

	if (suitableConverter != nullptr)
	{//we found a converter that accepts these kind of files
		path outputPath = a_outputDirectoryPath / a_relativeAssetPath.parent_path();
		if (!exists(outputPath))
		{//create any non-existing folder structures
			create_directories(outputPath);
		}
		string rawAssetStem = a_absoluteRawAssetPath.stem().string();
		path absoluteCookedAssetPath = outputPath / (rawAssetStem + suitableConverter->GetExtension());
		
		int32_t assetEntry = FindAssetEntry(a_relativeAssetPath.string().c_str());

		if (assetEntry != -1)
		{//existing asset entry found
			if (a_forceCook || g_assetEntries[assetEntry].second != a_assetSettings)
			{//asset settings have changed, need update
				Info("Updating asset from path: %s", a_absoluteRawAssetPath.string());
				if (!suitableConverter->CookAsset(a_absoluteRawAssetPath, absoluteCookedAssetPath, a_assetSettings))
				{//smth went wrong
					Error("Failed to update asset from path: %s\n  Check the log for details", a_absoluteRawAssetPath.string().c_str());
					return;
				}
			}
			else
			{
				Info("Skipped %s", a_absoluteRawAssetPath.string());
			}
		}
		else
		{//new file entry
			Info("Cooking new asset from path: %s", a_absoluteRawAssetPath.string());
			if (!suitableConverter->CookAsset(a_absoluteRawAssetPath, absoluteCookedAssetPath, a_assetSettings))
			{//smth went wrong
				Error("Failed to cook asset from path: %s\n  Check the log for details", a_absoluteRawAssetPath.string().c_str());
				return;
			}
		}

		FormatAndUpdateAssetEntry(
			a_relativeAssetPath, 
			suitableConverter->GetAssetType(), 
			suitableConverter->GetExtension(),
			a_assetSettings);

		Info("Finished cooking asset from %s", a_absoluteRawAssetPath.string());
	}
	else
	{
		Log("No suitable convert found for file: %s", a_absoluteRawAssetPath.string());
	}
}

void UncookAsset(
	const path& a_assetPath,
	const AssetEntry& a_assetEntry)
{
	AssetConverter* suitableConverter = nullptr;

	for (uint32_t i = 0; i < sizeof(g_converters) / sizeof(g_converters[0]); ++i)
	{
		path extension = a_assetEntry.m_extension;
		if (extension.string().length() > 8)
		{
			Error("Files that have an extension of more than 8 characters are not supported");
		}
		if (g_converters[i]->IsExtensionSupported(extension))
		{
			suitableConverter = g_converters[i];
			break;
		}
	}

	if (suitableConverter != nullptr)
	{//we found a converter that accepts these kind of files
		suitableConverter->UncookAsset(a_assetPath);
	}


}

int32_t FindActiveJobIndex(const path& jobPath)
{
	int32_t activeJobIndex = -1;
	for (int32_t i = 0; i < (int32_t)g_activeJobs.size(); ++i)
	{
		if (g_activeJobs[i].GetPath() == jobPath)
		{
			activeJobIndex = i;
			break;
		}
	}
	return activeJobIndex;
}

int32_t FindQueuedJobIndex(const path& jobPath)
{
	int32_t queuedJobIndex = -1;
	for (int32_t i = 0; i < (int32_t)g_queuedCookJobs.size(); ++i)
	{
		if (g_queuedCookJobs[i].GetPath() == jobPath)
		{
			queuedJobIndex = i;
			break;
		}
	}
	return queuedJobIndex;
}

PUG_RESULT pug::assets::InitAssetCooker(
	const path& a_assetBasePath, 
	const path& a_absoluteAssetOutputPath)
{


	g_assetOutputPath = a_absoluteAssetOutputPath;
	g_assetBasePath = a_assetBasePath;
	if (!exists(a_absoluteAssetOutputPath))
	{
		if (!create_directories(a_absoluteAssetOutputPath))
		{
			Error("Failed to create output directory at %s!", a_absoluteAssetOutputPath.string().c_str());
			return PUG_RESULT_FAILED_TO_CREATE_FILE_OR_DIR;
		}
	}

	ReadAssetEntriesFromFile();

	g_workerThreadSemaphore = CreateSemaphore(nullptr, 0, 0x7fffffff, nullptr);
	if (g_workerThreadSemaphore == nullptr)
	{
		Error("Failed to create worker thread semaphore");
		return PUG_RESULT_FAILED_TO_CREATE_OS_OBJECT;
	}
	g_cleanUpThreadSemaphore = CreateSemaphore(nullptr, 0, 0x7fffffff, nullptr);
	if (g_cleanUpThreadSemaphore == nullptr)
	{
		Error("Failed to create clean up thread semaphore");
		return PUG_RESULT_FAILED_TO_CREATE_OS_OBJECT;
	}
	g_cleanUpThread = (HANDLE)_beginthreadex(nullptr, 0, CleanUpThreadMain, nullptr, 0, nullptr);

	InitializeCriticalSection(&g_cookJobsGuard);
	InitializeCriticalSection(&g_activeJobsGuard);
	InitializeCriticalSection(&g_outputFileOperationsGuard);
	InitializeCriticalSection(&g_deleteGuard);

	g_threadsRunning = 1;
	for (uint32_t i = 0; i < NUM_WORKER_THREADS; ++i)
	{
		uint32_t threadID = 0;
		g_workerThreads[i] = (HANDLE)_beginthreadex(nullptr, 0, CookThreadMain , nullptr, 0, &threadID);
	}

	return PUG_RESULT_OK;
}

PUG_RESULT pug::assets::DestroyAssetCooker()
{
	//tell threads to die
	g_threadsRunning = 0;
	//allow threads to pass semaphore
	ReleaseSemaphore(g_workerThreadSemaphore, NUM_WORKER_THREADS, nullptr);
	ReleaseSemaphore(g_cleanUpThreadSemaphore, 1, nullptr);
	//wait for threads to die
	WaitForMultipleObjects(NUM_WORKER_THREADS, g_workerThreads, TRUE, INFINITE);
	WaitForSingleObject(g_cleanUpThread, INFINITE);

	CloseHandle(g_workerThreadSemaphore);
	CloseHandle(g_cleanUpThreadSemaphore);
	DeleteCriticalSection(&g_cookJobsGuard);
	DeleteCriticalSection(&g_activeJobsGuard);
	DeleteCriticalSection(&g_outputFileOperationsGuard);
	DeleteCriticalSection(&g_deleteGuard);

	for (uint32_t i = 0; i < PUG_COUNT_OF(g_converters); ++i)
	{
		delete g_converters[i];
	}

	return PUG_RESULT_OK;
}

PUG_RESULT pug::assets::SubmitCookJob(
	const std::experimental::filesystem::path& a_relativeAssetPath,
	const AssetSettings& a_assetSettings,
	const uint32_t a_forceCook /* = 0*/)
{
	PUG_RESULT res = PUG_RESULT_UNKNOWN;
	EnterCriticalSection(&g_cookJobsGuard);

	CookJob job = CookJob(a_relativeAssetPath, a_assetSettings, a_forceCook);
	//find if an older cookjob is still waiting
	int32_t cookJobIndex = -1;
	for (int32_t i = 0; i < (int32_t)g_queuedCookJobs.size(); ++i)
	{
		if (g_queuedCookJobs[i].GetPath() == a_relativeAssetPath)
		{//job with same path exists
			cookJobIndex = i;
			break;
		}
	}
	if (cookJobIndex != -1)
	{//replace existing cook job
		g_queuedCookJobs[cookJobIndex] = job;
	}
	else
	{//else push new job
		g_queuedCookJobs.push_back(job);
	}
	if (!ReleaseSemaphore(g_workerThreadSemaphore, (int32_t)g_queuedCookJobs.size(), nullptr))
	{
		res = PUG_RESULT_MUTEX_FAILURE;
	}
	LeaveCriticalSection(&g_cookJobsGuard);

	return res;
}

PUG_RESULT pug::assets::RemoveCookJob(const std::experimental::filesystem::path& a_relativeAssetPath)
{
	//find entry in asset entries
	//find if there was an output file already
		//remove output file
	//remove asset entry
	//flush new asset entry list to file
	EnterCriticalSection(&g_deleteGuard);
	g_deletedAssets.push_back(a_relativeAssetPath);
	LeaveCriticalSection(&g_deleteGuard);

	ReleaseSemaphore(g_cleanUpThreadSemaphore, 1, nullptr);

	return PUG_RESULT_OK;
}

bool pug::assets::IsJobActive(const std::experimental::filesystem::path& assetPath)
{
	return FindActiveJobIndex(assetPath) != -1;
}

bool pug::assets::IsJobQueued(const std::experimental::filesystem::path& assetPath)
{
	return FindQueuedJobIndex(assetPath) != -1;
}

AssetType pug::assets::DetermineAssetType(const path& assetPath)
{
	//PUG_ASSERT(assetPath.has_extension(), "no extensions detected!");
	for (uint32_t i = 0; i < PUG_COUNT_OF(g_converters); ++i)
	{
		if (g_converters[i]->IsExtensionSupported(assetPath.extension()))
		{
			return g_converters[i]->GetAssetType();
		}
	}
	return AssetType_Unknown;
}

void pug::assets::GetCopyOfActiveJobList(vector<path>& out_activeJobs)
{
	EnterCriticalSection(&g_cookJobsGuard);
	for (uint32_t i = 0; i < g_activeJobs.size(); ++i)
	{
		out_activeJobs.push_back(g_activeJobs[i].GetPath());
	}
	LeaveCriticalSection(&g_cookJobsGuard);
}

void pug::assets::GetCopyOfQueuedJobList(vector<path>& out_queuedJobs)
{
	EnterCriticalSection(&g_cookJobsGuard);
	for (uint32_t i = 0; i < g_queuedCookJobs.size(); ++i)
	{
		out_queuedJobs.push_back(g_queuedCookJobs[i].GetPath());
	}
	LeaveCriticalSection(&g_cookJobsGuard);
}

const uint32_t pug::assets::GetCookedAssetPath(
	const path& a_relativeRawAssetPath,
	path& out_absoluteCookedAssetPath)
{
	int32_t assetEntryIndex = FindAssetEntry(a_relativeRawAssetPath.string().c_str());
	if (assetEntryIndex != -1)
	{
		const path cookedExtension = g_assetEntries[assetEntryIndex].first.m_extension;
		out_absoluteCookedAssetPath = (g_assetOutputPath / a_relativeRawAssetPath).replace_extension(cookedExtension);
		return 1;
	}
	return 0;
}

uint32_t WINAPI CookThreadMain(void*)
{
	do
	{
		//wait till semaphore is released, this indicates new jobs
		WaitForSingleObject(g_workerThreadSemaphore, INFINITE);

		//lock acces to list
		EnterCriticalSection(&g_cookJobsGuard);
		CookJob cookJob = CookJob(path(), AssetSettings());

		int32_t activeJobIndex = -1;
		for(int32_t i = 0; i < (int32_t)g_queuedCookJobs.size(); ++i)
		{
			//see if a job with the same file name is present in the active job list
			EnterCriticalSection(&g_activeJobsGuard);
			activeJobIndex = FindActiveJobIndex(g_queuedCookJobs[i].GetPath());
			LeaveCriticalSection(&g_activeJobsGuard);
			if (activeJobIndex == -1)
			{//no active job with this file name
				//remove job from waiting queue, do this by swapping this job with the last one and popping back
				//make a local copy first
				cookJob = g_queuedCookJobs[i];
				//swap
				std::swap(g_queuedCookJobs[i], g_queuedCookJobs.back());
				//pop
				g_queuedCookJobs.pop_back();
				//safely add job to active list
				EnterCriticalSection(&g_activeJobsGuard);
				g_activeJobs.push_back(cookJob);
				LeaveCriticalSection(&g_activeJobsGuard);
				break;
			}
			else
			{//re-release our semaphore, so our threads keep going
				//Info("Active job detected for %s", g_activeJobs[activeJobIndex].GetPath().string());
				ReleaseSemaphore(g_workerThreadSemaphore, 1, nullptr);
			}
		} 
		LeaveCriticalSection(&g_cookJobsGuard);

		//we now have a local copy of a job that WAS not present in the active list, it is currently impossible to stop an active job
		if (cookJob.GetSettings().m_type != AssetType_Unknown)
		{
			path absoluteRawAssetPath = g_assetBasePath / cookJob.GetPath();
			CookAsset(
				absoluteRawAssetPath,
				g_assetOutputPath,
				cookJob.GetPath(),
				cookJob.GetSettings(),
				cookJob.IsForced());
		}
		//lock acces to active job list
		EnterCriticalSection(&g_activeJobsGuard);
		//remove the job from the active list
		for (int32_t i = 0; i < (int32_t)g_activeJobs.size(); ++i)
		{
			if (cookJob.GetPath() == g_activeJobs[i].GetPath())
			{
				std::swap(g_activeJobs[i], g_activeJobs.back());
				g_activeJobs.pop_back();
			}
		}
		//unlock acces to active job list;
		LeaveCriticalSection(&g_activeJobsGuard);
		WriteAssetEntriesToFile();

	}
	while(g_threadsRunning);

	_endthreadex(0);
	return 0;
}

uint32_t WINAPI CleanUpThreadMain(void*)
{
	do
	{
		//wait till semaphore is released, this indicates new delete requests
		WaitForSingleObject(g_cleanUpThreadSemaphore, INFINITE);

		//lock access to job list
		EnterCriticalSection(&g_cookJobsGuard);

			for(uint32_t i = 0; i < g_deletedAssets.size(); ++i)
			{
				//lock access to delete queue
				EnterCriticalSection(&g_deleteGuard);
					//make local copy of path
					path assetPath = g_deletedAssets[i];
				//unlock access to delete queue
				LeaveCriticalSection(&g_deleteGuard);

				//remove queued jobs to same file
				int32_t queuedJobIndex = FindQueuedJobIndex(assetPath);
				if (queuedJobIndex != -1)
				{//there was a queued job waiting
					std::swap(g_queuedCookJobs[queuedJobIndex], g_queuedCookJobs.back());
					g_queuedCookJobs.pop_back();
				}

				//wait for active jobs to finish
				int32_t activeJobIndex = -1;
				do
				{
					EnterCriticalSection(&g_activeJobsGuard);
					activeJobIndex = FindActiveJobIndex(assetPath);
					LeaveCriticalSection(&g_activeJobsGuard);
					if (activeJobIndex != -1)
					{//baaaaaaaaaaaaaaaaaaaaaaaaaaaaad
						Sleep(5);
					}
				}
				while(activeJobIndex != -1);

				EnterCriticalSection(&g_deleteGuard);
				int32_t assetEntryIndex = FindAssetEntry(assetPath.string().c_str());
				if (assetEntryIndex != -1)
				{
					AssetEntry assetEntry = g_assetEntries[assetEntryIndex].first;
					path absoluteCookedAssetPath = path();
					GetCookedAssetPath(assetPath, absoluteCookedAssetPath);
					//remove entry before uncooking, this prevents any other threads trying to access this delete job again
					std::swap(g_assetEntries[assetEntryIndex], g_assetEntries.back());
					g_assetEntries.pop_back();
					//uncook the asset
					UncookAsset(absoluteCookedAssetPath, assetEntry);
					//flush new asset entry list to file
					WriteAssetEntriesToFile();
				}
				//remove delete job
				std::swap(g_deletedAssets[i], g_deletedAssets.back());
				g_deletedAssets.pop_back();

				LeaveCriticalSection(&g_deleteGuard);
			}

		//unlock access to job list
		LeaveCriticalSection(&g_cookJobsGuard);
	}
	while(g_threadsRunning);

	_endthreadex(0);
	return 0;
}