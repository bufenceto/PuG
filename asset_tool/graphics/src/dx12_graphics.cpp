#include "graphics.h"

#include "dx12_helper.h"
#include "committed_descriptor_heap.h"
#include "dx12_texture2D.h"

#include "windows_window.h"

#include <d3d12.h>
#include <dxgi1_6.h>

using namespace pug;
using namespace pug::assets;
using namespace pug::assets::graphics;
using namespace pug::log;
using namespace pug::platform;
using namespace pug::windows;

static ID3D12Debug* g_debugInterface;
static ID3D12Device* g_device;
static ID3D12DebugDevice* g_debugDevice;

static ID3D12DebugCommandQueue* g_debugCommandQueue;
static ID3D12CommandQueue* g_commandQueue;

static IDXGISwapChain3* g_swapchain;
static const uint32_t bufferCount = 2;

#define BACK_BUFFER_FORMAT DXGI_FORMAT_R8G8B8A8_UNORM

PUG_RESULT pug::assets::graphics::InitGraphics(
	pug::platform::Window* a_window,
	uint32_t a_verticalSyncInterval,
	uint32_t a_fullscreen)
{
	WindowsWindow* windowsWindow = dynamic_cast<WindowsWindow*>(a_window);
	if (windowsWindow == nullptr)
	{
		Error("Tried to initialize DX12 with a non windows window!");
	}


	//if we are debugging we want an active debug layer
	HRESULT result;
#ifdef _DEBUG// || defined(DX12_DEBUG)
	result = D3D12GetDebugInterface(IID_PPV_ARGS(&g_debugInterface));
	if (FAILED(result))
	{
		Error("Failed to get D3D12 Debug interface.");
		return 1;
	}
	g_debugInterface->EnableDebugLayer();
#endif

	IDXGIFactory1* dxgiFactory = nullptr;
	IDXGIAdapter1* adapter = nullptr;
	if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory))))
	{
		Error("Failed to create DXGI factory!");
		return PUG_RESULT_GRAPHICS_ERROR;
	}


	if (!FindAdapter(dxgiFactory, adapter))
	{
		Error("Failed to find suitable adapter!");
		return PUG_RESULT_GRAPHICS_ERROR;
	}
	LogAdapterStats(adapter);

	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};

	if (!CreateDeviceForHighestFeatureLevel(featureLevels, PUG_COUNT_OF(featureLevels), adapter, g_device, g_debugDevice))
	{
		Error("Failed To Create DX12 device!");
		return PUG_RESULT_GRAPHICS_ERROR;
	}

	if (!CreateCommandQueue(
		g_device,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
		D3D12_COMMAND_QUEUE_FLAG_NONE,
		g_commandQueue,
		g_debugCommandQueue))
	{
		Error("Failed to create swapchain!");
		return PUG_RESULT_GRAPHICS_ERROR;
	}


	if (!CreateSwapchain(
		dxgiFactory,
		adapter,
		g_commandQueue,
		windowsWindow->GetWindowHandle(),
		BACK_BUFFER_FORMAT,
		windowsWindow->GetSize(),
		bufferCount,
		a_fullscreen,
		a_verticalSyncInterval,
		g_swapchain))
	{
		Error("Failed to create swapchain!");
		return PUG_RESULT_GRAPHICS_ERROR;
	}

	PUG_TRY(InitCommittedResourceHeap(
		g_device, 
		128, 
		8, 
		8, 
		128, 
		128));
	
	TextureHandle testHandle = {0};

	PUG_TRY(AllocateTexture2D(
		nullptr,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		vmath::Vector4(0),
		D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
		1280,
		720,
		D3D12_RESOURCE_STATE_COMMON,
		testHandle));

	//PUG_TRY(ReleaseTexture2D(testHandle));
		
	return PUG_RESULT_OK;
}

PUG_RESULT pug::assets::graphics::DestroyGraphics()
{
	DestroyCommittedResourceHeap();

	SafeRelease(g_device);
	SafeRelease(g_debugDevice);
	SafeRelease(g_debugInterface);

	return PUG_RESULT_OK;
}

