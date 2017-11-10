#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <cstdint>

#include <string>
#include <codecvt>

#include "../logger/logger.h"
#include "core/inc/macro.h"
#include "formats.h"

#include "vmath.h"

using namespace pug;
using namespace pug::assets;
using namespace pug::assets::graphics;

#define SET_NAME(a) a.SetName(L#a)

static const size_t FormatToSize(const DXGI_FORMAT format)
{
	switch (format)
	{
	case(DXGI_FORMAT_R16_UINT): return 2;
	case(DXGI_FORMAT_R32_UINT): return 4;
	default: return 0;
	}
}

static DXGI_FORMAT ConvertFormat(PUG_FORMAT format)
{
	switch (format)
	{
	case PUG_FORMAT_R16_UINT: return DXGI_FORMAT_R16_UINT;
	case PUG_FORMAT_R32_UINT: return DXGI_FORMAT_R32_UINT;

	case PUG_FORMAT_RGBA8_UINT: return DXGI_FORMAT_R8G8B8A8_UINT;
	case PUG_FORMAT_RGBA16_UINT: return DXGI_FORMAT_R16G16B16A16_UINT;

	case PUG_FORMAT_D24S8: return DXGI_FORMAT_D24_UNORM_S8_UINT;

	case PUG_FORMAT_RGB10A2: return DXGI_FORMAT_R10G10B10A2_TYPELESS;
	case PUG_FORMAT_RG11B10F: return DXGI_FORMAT_R11G11B10_FLOAT;
	case PUG_FORMAT_RGBA16F: return DXGI_FORMAT_R16G16B16A16_FLOAT;
	case PUG_FORMAT_RGBA32F: return DXGI_FORMAT_R32G32B32A32_FLOAT;
	case PUG_FORMAT_R16F: return DXGI_FORMAT_R16_FLOAT;
	case PUG_FORMAT_R32F: return DXGI_FORMAT_R32_FLOAT;
	case PUG_FORMAT_BC3_UNORM: return DXGI_FORMAT_BC3_UNORM;
	default:
		return DXGI_FORMAT_UNKNOWN;
	}
}

// Safely release a COM object.
template<typename T>
inline void SafeRelease(T& ptr)
{
	if (ptr != NULL)
	{
		ptr->Release();
		ptr = NULL;
	}
}

static std::string FeatureLevelToString(const D3D_FEATURE_LEVEL& featureLevel)
{
	switch (featureLevel)
	{
	case D3D_FEATURE_LEVEL_12_1: return "D3D_FEATURE_LEVEL_12_1";
	case D3D_FEATURE_LEVEL_12_0: return "D3D_FEATURE_LEVEL_12_0";
	case D3D_FEATURE_LEVEL_11_1: return "D3D_FEATURE_LEVEL_11_1";
	case D3D_FEATURE_LEVEL_11_0: return "D3D_FEATURE_LEVEL_11_0";
	case D3D_FEATURE_LEVEL_10_1: return "D3D_FEATURE_LEVEL_10_1";
	case D3D_FEATURE_LEVEL_10_0: return "D3D_FEATURE_LEVEL_10_0";
	case D3D_FEATURE_LEVEL_9_3:  return "D3D_FEATURE_LEVEL_9_3";
	case D3D_FEATURE_LEVEL_9_2:  return "D3D_FEATURE_LEVEL_9_2";
	case D3D_FEATURE_LEVEL_9_1:  return "D3D_FEATURE_LEVEL_9_1";
	default: return "Unknown feature level!";
	}
}
static std::string ws2s(const std::wstring& wstr)
{
	using convert_type = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_type, wchar_t> converter;

	return converter.to_bytes(wstr);
}

static bool LogAdapterStats(IDXGIAdapter1* adapter)
{
	DXGI_ADAPTER_DESC1 adapterDesc = {};
	// Get the adapter (video card) description.
	if (FAILED(adapter->GetDesc1(&adapterDesc)))
	{
		log::Error("Failed to get adapter description!");
		return false;
	}

	// Store the dedicated video card memory in megabytes.
	uint32_t vram = (uint32_t)(MB(adapterDesc.DedicatedVideoMemory));

	//print some info in the graphics card we are using
	// Convert the name of the video card to a character array and store it.
	std::string desc = ws2s(adapterDesc.Description);
	uint64_t dedVidRAM = adapterDesc.DedicatedVideoMemory;
	uint64_t dedSysRam = adapterDesc.DedicatedSystemMemory;
	log::Info("Adapter: %s", desc.c_str());
	log::Info("Dedicated video memory(mb): %d", dedVidRAM >> 20);
	log::Info("Dedicated system memory(mb): %d", dedSysRam >> 20);
	return true;
}

static IDXGIAdapter1* FindHardwareAdapterWithMostDedicatedMemory(
	IDXGIAdapter1** adapters,
	uint32_t adapterCount)
{
	DXGI_ADAPTER_DESC1 adapterDesc = {};
	size_t mem = 0;
	uint32_t chosenAdapterIndex = -1;
	for (uint32_t i = 0; i < adapterCount; ++i)
	{
		adapters[i]->GetDesc1(&adapterDesc);
		if ((adapterDesc.Flags & D3D_DRIVER_TYPE_SOFTWARE) || //this is a software device
			(adapterDesc.Flags & D3D_DRIVER_TYPE_WARP) || //this is a fallback warp device
			adapterDesc.DeviceId == 0x8c) //this is the standard windows rasterizer
		{
			continue;
		}
		if (adapterDesc.DedicatedVideoMemory > mem)
		{
			mem = adapterDesc.DedicatedVideoMemory;
			chosenAdapterIndex = i;
		}
	}
	if (chosenAdapterIndex != -1)
	{
		return adapters[chosenAdapterIndex];
	}
	else
	{
		return nullptr;
	}
}

static bool FindAdapter(
	IDXGIFactory1* factory,
	IDXGIAdapter1*& out_result)
{
	HRESULT hr;
	// Use the factory to fill an array with all available adapters
	IDXGIAdapter1* adapters[8] = {};
	uint32_t adapterCount = 0;
	for (uint32_t i = 0; i < PUG_COUNT_OF(adapters); ++i)
	{
		hr = factory->EnumAdapters1(i, &adapters[i]);
		if (FAILED(hr))
		{
			adapterCount = i;
			break;
		}
	}
	//we dont want a software or warp adapter
	out_result = FindHardwareAdapterWithMostDedicatedMemory(adapters, adapterCount);

	for (uint32_t i = 0; i < PUG_COUNT_OF(adapters); ++i)
	{
		if (adapters[i] != nullptr && adapters[i] != out_result)
		{
			adapters[i]->Release();
			adapters[i] = nullptr;
		}
	}
	return out_result != nullptr;
}

static bool CreateDeviceForHighestFeatureLevel(
	const D3D_FEATURE_LEVEL* featureLevels,
	const uint32_t featureLevelCount,
	IDXGIAdapter1* adapter,
	ID3D12Device*& out_device,
	ID3D12DebugDevice*& out_debugDevice)
{
	void** devicePointer = nullptr;
#ifdef _DEBUG
	devicePointer = (void**)&out_debugDevice;
#else
	devicePointer = (void**)&out_device;
#endif

	//find highest feature level
	int32_t selectedFeatureLevelIndex = -1;
	for (uint32_t i = 0; i < featureLevelCount; ++i)
	{
		if (!FAILED(D3D12CreateDevice(adapter, featureLevels[i], __uuidof(ID3D12Device), nullptr)))
		{
			std::string featureLevelString = FeatureLevelToString(featureLevels[i]);
			log::Info("Picking feature level %s", featureLevelString.c_str());
			selectedFeatureLevelIndex = i;
			break;
		}
	}
	if (selectedFeatureLevelIndex != -1)
	{
		if (!FAILED(D3D12CreateDevice(adapter, featureLevels[selectedFeatureLevelIndex], __uuidof(ID3D12Device), devicePointer)))
		{
			log::Message("Succesfully created device for chosen feature level");
		}
		else
		{
			log::Error("Failed to create a DirectX12 device");
		}
	}
	else
	{
		log::Error("Failed to find suitable feature level");
	}



#ifdef _DEBUG
	if (FAILED(out_debugDevice->QueryInterface(IID_PPV_ARGS(&out_device))))
	{
		log::Error("Failed to promote debug device to device");
		return false;
	}
#endif
	return true;
}

static bool CreateCommandQueue(
	ID3D12Device* device,
	D3D12_COMMAND_LIST_TYPE type,
	D3D12_COMMAND_QUEUE_PRIORITY priority,
	D3D12_COMMAND_QUEUE_FLAGS flags,
	ID3D12CommandQueue*& out_commandQueue,
	ID3D12DebugCommandQueue*& out_debugCommandQueue)
{
	if (device == nullptr)
	{
		log::Error("Device is not initialized!");
		return false;
	}
	//COMMAND QUEUE
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
	commandQueueDesc.Type = type;
	commandQueueDesc.Priority = priority;
	commandQueueDesc.Flags = flags;
	commandQueueDesc.NodeMask = 0;

	void** commandQueuePointer = nullptr;
#ifdef _DEBUG
	commandQueuePointer = (void**)&out_debugCommandQueue;
#else
	commandQueuePointer = (void**)&out_commandQueue;
#endif
	if (FAILED(device->CreateCommandQueue(&commandQueueDesc, __uuidof(ID3D12CommandQueue), commandQueuePointer)))
	{
		log::Error("Failed to create command queue");
		return false;
	}
#ifdef _DEBUG
	if (FAILED(out_debugCommandQueue->QueryInterface(IID_PPV_ARGS(&out_commandQueue))))
	{
		log::Error("Failed to promote debug command queue to command queue");
		return false;
	}
#endif
	return true;
}

static bool CreateSwapchain(
	IDXGIFactory1* factory,
	IDXGIAdapter1* adapter,
	ID3D12CommandQueue* commandQueue,
	const HWND& windowHandle,
	DXGI_FORMAT backBufferFormat,
	const vmath::Int2& windowSize,
	const uint32_t backBufferCount,
	uint32_t fullScreen,
	uint32_t vSyncInterval,
	IDXGISwapChain3*& out_result)
{
	if (factory == nullptr || adapter == nullptr || commandQueue == nullptr)
	{
		log::Error("Adapter or Factory was not initialized");
		return false;
	}

	IDXGIOutput* adapterOutput[8] = {};
	IDXGISwapChain* tmpSwapChain = nullptr;
	uint32_t numModes;
	DXGI_MODE_DESC* displayModeList = nullptr;
	uint32_t numerator = 0;
	uint32_t denominator = 1;

	uint32_t enumCount = 0;
	for (uint32_t i = 0; i < PUG_COUNT_OF(adapterOutput); ++i)
	{
		HRESULT hr = adapter->EnumOutputs(i, &adapterOutput[i]);
		if (hr == DXGI_ERROR_NOT_FOUND)
		{
			enumCount = i;
			break;
		}
	}

	if (fullScreen)
	{
		for (uint32_t i = 0; i < enumCount; ++i)
		{
			// Get the number of modes that fit the display format for the adapter output (monitor).
			if (FAILED(adapterOutput[i]->GetDisplayModeList(backBufferFormat, DXGI_ENUM_MODES_INTERLACED, &numModes, NULL)))
			{
				log::Error("Failed to get display mode list count!");
				return false;
			}

			// Create a list to hold all the possible display modes for this monitor/video card combination.
			displayModeList = (DXGI_MODE_DESC*)_malloca(sizeof(DXGI_MODE_DESC) * numModes);// new DXGI_MODE_DESC[numModes];
			if (!displayModeList)
			{
				log::Error("Failed to allocate memory for display mode list!");
				return false;
			}

			// Now fill the display mode list structures.
			if (FAILED(adapterOutput[i]->GetDisplayModeList(backBufferFormat, DXGI_ENUM_MODES_INTERLACED, &numModes, displayModeList)))
			{
				log::Error("Failed to fill display mode list!");
				return false;
			}

			// Now go through all the display modes and find the one that matches the screen height and width.
			// When a match is found store the numerator and denominator of the refresh rate for that monitor.
			for (uint32_t i = 0; i < numModes; ++i)
			{
				if (displayModeList[i].Height == (uint32_t)windowSize.y)
				{
					if (displayModeList[i].Width == (uint32_t)windowSize.x)
					{
						numerator = displayModeList[i].RefreshRate.Numerator;
						denominator = displayModeList[i].RefreshRate.Denominator;
						break;
					}
				}
			}
		}

		for (uint32_t i = 0; i < enumCount; ++i)
		{
			// Release the adapter output.
			adapterOutput[i]->Release();
			adapterOutput[i] = nullptr;
		}
	}

	// Initialize the swap chain description.
	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	// Set the swap chain to use double buffering.
	swapChainDesc.BufferCount = backBufferCount;
	// Set the height and width of the back buffers in the swap chain.
	swapChainDesc.BufferDesc.Width = windowSize.x;
	swapChainDesc.BufferDesc.Height = windowSize.y;
	// Set a regular 32-bit surface for the back buffers.
	swapChainDesc.BufferDesc.Format = backBufferFormat;
	// Set the usage of the back buffers to be render target outputs.
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	// Set the swap effect to discard the previous buffer contents after swapping.
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	// Set the handle for the window to render to.
	swapChainDesc.OutputWindow = windowHandle;

	swapChainDesc.Windowed = !fullScreen;
	swapChainDesc.BufferDesc.RefreshRate.Numerator = numerator;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = denominator;

	// Turn multisampling off.
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;

	// Set the scan line ordering and scaling to unspecified.
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	// Don't set the advanced flags.
	swapChainDesc.Flags = 0;

	// Finally create the swap chain using the swap chain description.	
	HRESULT hr = factory->CreateSwapChain(commandQueue, &swapChainDesc, &tmpSwapChain);
	if (FAILED(hr))//
	{
		log::Error("Failed to create swap chain!");
		return false;
	}

	// Next upgrade the IDXGISwapChain to a IDXGISwapChain3 interface and store it in a private member variable named m_swapChain.
	// This will allow us to use the newer functionality such as getting the current back buffer index.
	if (FAILED(tmpSwapChain->QueryInterface(__uuidof(IDXGISwapChain3), (void**)&out_result)))
	{
		log::Error("Failed to upgrade swap chain interface!");
		return false;
	}

	// Clear pointer to original swap chain interface since we are using version 3 instead (m_swapChain).
	tmpSwapChain->Release();
	tmpSwapChain = nullptr;

	return true;
}