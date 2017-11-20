
#include "dx12_shader_librarian.h"

#include "macro.h"
#include "defines.h"

#include "hash.h"
#include "logger.h"

#include <experimental/filesystem>
#include <Windows.h>
#include <process.h>
#include <D3Dcompiler.h>

#define MAX_SHADERS 64
#define MAX_SHADER_COMPILE_TRY_COUNT 5

#ifdef _DEBUG
#define COMPILE_FLAGS D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_WARNINGS_ARE_ERRORS
#else
#define COMPILE_FLAGS D3DCOMPILE_DEBUG | D3DCOMPILE_WARNINGS_ARE_ERRORS | D3DCOMPILE_OPTIMIZATION_LEVEL3
#endif

using namespace std;
using namespace std::experimental::filesystem;
using namespace pug;
using namespace pug::assets::graphics;
using namespace pug::log;

struct ShaderEntry
{
	char shaderGUID[SHA1_HASH_BYTES];
	uint32_t isInitialized;
};//28 bytes

static ShaderEntry g_loadedShaders[MAX_SHADERS];

static path g_paths[MAX_SHADERS];
static uint8_t* g_byteCodes[MAX_SHADERS];
static uint64_t g_byteCodeSizes[MAX_SHADERS];
static file_time_type g_lastWriteTimes[MAX_SHADERS];

static HANDLE g_thread;
static CallBack g_callback;
static path g_shaderFileDirectory;

string DetermineShaderTargetType(const string& fileName)
{
	if (fileName.find(".vs") != string::npos)
	{
		return "vs_5_0";
	}
	else if (fileName.find(".ps") != string::npos)
	{
		return "ps_5_0";
	}
	else if (fileName.find(".gs") != string::npos)
	{
		return "gs_5_0";
	}
	else if (fileName.find(".ds") != string::npos)
	{
		return "ds_5_0";
	}
	else if (fileName.find(".cs") != string::npos)
	{
		return "cs_5_0";
	}
	else
	{
		return string();
	}
}

PUG_RESULT LoadShader(
	const path& absoluteFilePath,
	const string& fileName)
{
	PUG_RESULT res = PUG_RESULT_OK;
	uint32_t index = 0;
	if (exists(absoluteFilePath))
	{
		//scan for available shader slot
		for (uint32_t i = 0; i < MAX_SHADERS; ++i)
		{
			if (!g_loadedShaders[i].isInitialized && i != PUG_INVALID_ID)
			{
				index = i;
				break;
			}
		}
		if (index != PUG_INVALID_ID)
		{
			uint64_t hlslFileSize = file_size(absoluteFilePath);
			uint8_t* hlslFileBuffer = (uint8_t*)_malloca(hlslFileSize);//stack alloc for the file character array
			if (hlslFileBuffer == nullptr)
			{
				return PUG_RESULT_ALLOCATION_FAILURE;
			}

			FILE* shaderFile = nullptr;
			fopen_s(&shaderFile, absoluteFilePath.string().c_str(), "rb");
			uint64_t bytesRead = fread(hlslFileBuffer, sizeof(uint8_t), hlslFileSize, shaderFile);
			if (bytesRead == hlslFileSize)
			{//succes
				ID3DBlob* blob = nullptr;
				ID3DBlob* errorBlob = nullptr;
				string shaderTargetType = DetermineShaderTargetType(fileName);
				D3DCompile(
					hlslFileBuffer, 
					hlslFileSize, 
					absoluteFilePath.string().c_str(), 
					nullptr, 
					D3D_COMPILE_STANDARD_FILE_INCLUDE, 
					"main", 
					shaderTargetType.c_str(), 
					COMPILE_FLAGS, 
					0, 
					&blob, 
					&errorBlob);
				if (blob != nullptr)
				{
					uint64_t blobSize = blob->GetBufferSize();
					g_byteCodes[index] = (uint8_t*)_aligned_malloc(blobSize, 16);
					////copy blob data to our own buffer
					memcpy(g_byteCodes[index], blob->GetBufferPointer(), blobSize);//g_byteCodes[index] = blob->GetBufferPointer();
					////store buffer size;
					g_byteCodeSizes[index] = blobSize;
					////write hash
					utility::SHA1(fileName, g_loadedShaders[index].shaderGUID, sizeof(g_loadedShaders[index].shaderGUID));
					////reserve slot
					g_loadedShaders[index].isInitialized = 1;
					////store absolute filepath for reloading
					g_paths[index] = absoluteFilePath;
					////store our write time, we use this to determine which files have changed and need reloading
					g_lastWriteTimes[index] = last_write_time(absoluteFilePath);
					////release blob
					blob->Release();
				}
				else
				{
					Error("Failed to compile shader %s.\nError msg: %s", fileName.c_str(), (char*)errorBlob->GetBufferPointer());
					errorBlob->Release();
					errorBlob = nullptr;
					res = PUG_RESULT_FAILED_TO_CREATE_GPU_RESOURCE;
				}
			}
			else
			{//clean up
				res = PUG_RESULT_FAILED_TO_READ_FILE;
			}
			fclose(shaderFile);
		}
		else
		{
			res = PUG_RESULT_ALLOCATION_FAILURE;
		}
	}
	return res;
}

bool UnloadShader(const uint32_t index)
{
	_aligned_free(g_byteCodes[index]);
	g_byteCodeSizes[index] = 0;
	g_paths[index] = "";

	PUG_ZERO_MEM(g_loadedShaders[index].shaderGUID);
	g_loadedShaders[index].isInitialized = 0;

	return true;
}

PUG_RESULT ReloadShader(uint32_t index)
{
	PUG_RESULT res = PUG_RESULT_OK;
	const path& filePath = g_paths[index];
	uint64_t hlslFileSize = file_size(filePath);
	uint8_t* hlslFileBuffer = (uint8_t*)_malloca(hlslFileSize);

	FILE* shaderFile = nullptr;
	errno_t openRes = fopen_s(&shaderFile, g_paths[index].string().c_str(), "rb");
	if (openRes == 0)
	{
		uint64_t bytesRead = fread(hlslFileBuffer, sizeof(uint8_t), hlslFileSize, shaderFile);
		if (bytesRead == hlslFileSize && hlslFileSize != 0)
		{//succes
			ID3DBlob* blob = nullptr;
			ID3DBlob* errorBlob = nullptr;
			string shaderTargetType = DetermineShaderTargetType(g_paths[index].string());
			D3DCompile(hlslFileBuffer, hlslFileSize, filePath.string().c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", shaderTargetType.c_str(), COMPILE_FLAGS, 0, &blob, &errorBlob);
			if (blob != nullptr)
			{//succes
				//release old bytecode
				_aligned_free(g_byteCodes[index]);
				//allocate new buffer
				uint64_t blobSize = blob->GetBufferSize();
				g_byteCodes[index] = (uint8_t*)_aligned_malloc(blobSize, 16);
				//copy blob data to our own buffer
				memcpy(g_byteCodes[index], blob->GetBufferPointer(), blobSize);//g_byteCodes[index] = blob->GetBufferPointer();
				//store buffer size;
				g_byteCodeSizes[index] = blobSize;
				//store our write time, we use this to determine which files have changed and need reloading
				g_lastWriteTimes[index] = last_write_time(g_paths[index]);
				////release blob
				blob->Release();
			}
			else
			{
				Warning("Failed to re-compile shader %s.\nError msg: %s", g_paths[index].string().c_str(), (char*)errorBlob->GetBufferPointer());
				errorBlob->Release();
				errorBlob = nullptr;
				res = PUG_RESULT_FAILED_TO_CREATE_GPU_RESOURCE;
			}
		}
		else
		{//clean up
			res = PUG_RESULT_FAILED_TO_READ_FILE;
		}
		if (fclose(shaderFile) != 0)
		{
			res = PUG_RESULT_FAILED_TO_CLOSE_FILE;
		}
	}
	return res;
}

uint32_t WINAPI ShaderLibrarianMain(void*)
{
	HANDLE changeNotificationObject = FindFirstChangeNotificationA(
		g_shaderFileDirectory.string().c_str(),
		TRUE,
		FILE_NOTIFY_CHANGE_LAST_WRITE);
	PUG_ASSERT(changeNotificationObject, "Failed to create change notification object");

	do//main loop
	{
		//we want to immediatly return
		DWORD res = WaitForSingleObject(changeNotificationObject, 100);
		if (res == WAIT_OBJECT_0)
		{//something changed
			if (!FindNextChangeNotification(changeNotificationObject))
			{
				Error("System reported a change but a change could not be found");
			}

			for (uint32_t i = 0; i < PUG_COUNT_OF(g_loadedShaders); ++i)
			{
				if (g_loadedShaders[i].isInitialized)
				{
					uint32_t tryCount = 0;
					std::error_code ec;
					do
					{
						file_status shaderFileStatus = status(g_paths[i]);
						file_time_type newWriteTime = last_write_time(g_paths[i], ec);
						if (exists(g_paths[i]) && ec.value() == 0)//some programs may remove the file entry before re-adding it
						{
							if ((newWriteTime > g_lastWriteTimes[i]))
							{
								PUG_TRY(ReloadShader(i));
							}
							break;
						}
						else
						{
							++tryCount;
							Sleep(100);
						}
					}
					while(tryCount <= MAX_SHADER_COMPILE_TRY_COUNT);
					if (tryCount > MAX_SHADER_COMPILE_TRY_COUNT)
					{
						Info("Failed to recompile shader %s", g_paths[i].string().c_str());
					}
				}
			}

			if (g_callback != nullptr)
			{//use callback to notify system that new shader code is ready
				g_callback();
			}
		}
	}
	while(1);

	PUG_ASSERT(CloseHandle(changeNotificationObject), "Failed to close change notification object handle");
}

PUG_RESULT pug::assets::graphics::InitShaderLibrarian()
{
	path shaderFolderPath = canonical(current_path() / "graphics/rsc/");
	log::Info("Loading shaders from %s", shaderFolderPath.string().c_str());
	size_t len = shaderFolderPath.string().length();
	//load shaders in the directory
	recursive_directory_iterator it = recursive_directory_iterator(shaderFolderPath);
	for (recursive_directory_iterator end = recursive_directory_iterator(); it != end; ++it)
	{
		directory_entry dirEntry = *it;
		string absolutePath = dirEntry.path().string();
		if (!is_directory(dirEntry))
		{//this is a file
			path filePath = dirEntry.path();
			if (filePath.extension() == ".hlsl")
			{//this is a hlsl file
				auto relBeg = absolutePath.begin() + (len + 1);//+1 for the additional seperator
				auto relEnd = absolutePath.end();
				if (PUG_SUCCEEDED(LoadShader(filePath, string(relBeg, relEnd))))
				{
					Info("Loaded shader from path: %s", filePath.string().c_str());
				}
				else
				{
					Error("Failed to load shader from path: %s", filePath.string().c_str());
				}
			}
		}
	}

	g_shaderFileDirectory = shaderFolderPath;
	uint32_t threadID;//what is this?
	g_thread = (HANDLE)_beginthreadex(nullptr, 0, ShaderLibrarianMain, nullptr, 0, &threadID);
	if (g_thread == 0)
	{
		return PUG_RESULT_PLATFORM_ERROR;
	}

	return PUG_RESULT_OK;
}

PUG_RESULT pug::assets::graphics::DestroyShaderLibrarian()
{
	if(!CloseHandle(g_thread))
	{
		return PUG_RESULT_PLATFORM_ERROR;
	}

	for (uint32_t i = 0; i < PUG_COUNT_OF(g_loadedShaders); ++i)
	{
		if (g_loadedShaders[i].isInitialized)
		{
			UnloadShader(i);
		}
	}

	return PUG_RESULT_OK;
}

void pug::assets::graphics::RegisterShaderReloadedCallback(CallBack callback)
{
	g_callback = callback;
}

bool pug::assets::graphics::GetShader(
	const path& shaderFileName,
	uint8_t*& out_byteCode, 
	uint64_t& out_byteCodeSize)
{
	char hash[SHA1_HASH_BYTES] = {};
	utility::SHA1(shaderFileName.string(), hash, sizeof(hash));
	uint32_t index = PUG_INVALID_ID;
	for (uint32_t i = 0; i < PUG_COUNT_OF(g_loadedShaders); ++i)
	{
		if (memcmp(g_loadedShaders[i].shaderGUID, hash, sizeof(hash)) == 0)
		{//found the shader
			index = i;
			break;
		}
	}

	if (index != PUG_INVALID_ID)
	{
		out_byteCode = g_byteCodes[index];
		out_byteCodeSize = g_byteCodeSizes[index];
		return true;
	}
	else
	{
		out_byteCode = nullptr;
		out_byteCodeSize = 0;
		return false;
	}
}

