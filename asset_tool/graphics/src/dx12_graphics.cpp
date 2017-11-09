#include "graphics.h"

#include "pug_funcs.h"

#include "dx12_helper.h"
#include "committed_descriptor_heap.h"
#include "dx12_texture2D.h"

#include "windows_window.h"

#include <d3d12.h>
#include <dxgi1_6.h>
#include "d3dx12.h"

using namespace pug;
using namespace pug::assets;
using namespace pug::assets::graphics;
using namespace pug::log;
using namespace pug::platform;
using namespace pug::windows;

using namespace vmath;

#define MAX_TEXTURES 512

static const uint32_t g_backBufferCount = 2;

static uint32_t g_currentBackBufferIndex;

static DX12Texture2D g_textures[MAX_TEXTURES];

static ID3D12Debug* g_debugInterface;
static ID3D12Device* g_device;
static ID3D12DebugDevice* g_debugDevice;

static ID3D12DebugCommandQueue* g_debugCommandQueue;
static ID3D12CommandQueue* g_commandQueue;

static ID3D12Fence* g_fence;
static uint32_t g_fenceValue;
static HANDLE g_fenceEvent;

static IDXGISwapChain3* g_swapchain;

static ID3D12CommandAllocator* g_commandAllocators[g_backBufferCount];
static ID3D12GraphicsCommandList* g_commandList;

static DX12Texture2D g_backBuffers[g_backBufferCount];

static Vector3 g_clearColor = RIGHT;

#define BACK_BUFFER_FORMAT DXGI_FORMAT_R8G8B8A8_UNORM

PUG_RESULT MapBackBuffersToTextures(
	ID3D12Device* a_device,
	IDXGISwapChain3* a_swapChain,
	DX12Texture2D* g_backBufferTextures,
	const uint32_t backBufferCount,
	const vmath::Int2& screenSize)
{
	HRESULT result;

	for (uint32_t i = 0; i < backBufferCount; ++i)
	{
		ID3D12Resource* backBuffer = nullptr;
		result = a_swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer));//write the pointer of our back buffer to our textures
		if (FAILED(result))
		{
			Error("Failed to get pointer to back buffer in swap chain with index: %d!", i);
			return PUG_RESULT_GRAPHICS_ERROR;
		}
		uint32_t rtvDescriptorIndex = 0;
		//rtv
		{
			D3D12_CPU_DESCRIPTOR_HANDLE cpuRTVHandle = {};
			D3D12_GPU_DESCRIPTOR_HANDLE gpuRTVHandle = {};
			
			PUG_TRY(AllocateComittedRTVDescriptors(rtvDescriptorIndex));
			PUG_TRY(GetCommittedRTVDescriptors(rtvDescriptorIndex, cpuRTVHandle, gpuRTVHandle));
			a_device->CreateRenderTargetView(backBuffer, nullptr, cpuRTVHandle);
		}

		g_backBufferTextures[i] = DX12Texture2D(
			backBuffer,
			0,
			0,
			rtvDescriptorIndex,
			0,
			0,
			screenSize.x,
			screenSize.y,
			BACK_BUFFER_FORMAT,
			D3D12_RESOURCE_STATE_PRESENT);
	}

	return true;
}

PUG_RESULT RecordCommandList()
{
	HRESULT result;

	result = g_commandList->Reset(g_commandAllocators[g_currentBackBufferIndex], nullptr);
	if (FAILED(result))
	{
		Error("Failed to reset command list for recording");
		return false;

	}

	D3D12_RESOURCE_BARRIER barriersIn[] =
	{
		g_backBuffers[g_currentBackBufferIndex].Transition(D3D12_RESOURCE_STATE_RENDER_TARGET),
	};

	g_commandList->ResourceBarrier(PUG_COUNT_OF(barriersIn), barriersIn);

	D3D12_CPU_DESCRIPTOR_HANDLE cpuRenderTargetRTV = {};
	D3D12_GPU_DESCRIPTOR_HANDLE gpuRenderTargetRTV = {};
	PUG_TRY(GetCommittedRTVDescriptors(g_backBuffers[g_currentBackBufferIndex].GetRTVHeapIndex(), cpuRenderTargetRTV, gpuRenderTargetRTV));
	
	g_clearColor = g_clearColor * Matrix3(Vector3(RADIANS(45.0f * GetDeltaTime())));

	const float clearColor[] = { g_clearColor.x, g_clearColor.y, g_clearColor.z, 0.0f };
	g_commandList->ClearRenderTargetView(cpuRenderTargetRTV, clearColor, 0, nullptr);

	D3D12_RESOURCE_BARRIER barriersOut[] =
	{
		g_backBuffers[g_currentBackBufferIndex].Transition(D3D12_RESOURCE_STATE_PRESENT),
	};

	g_commandList->ResourceBarrier(PUG_COUNT_OF(barriersOut), barriersOut);

	result = g_commandList->Close();
	if (FAILED(result))
	{
		Error("Failed to close command list");
		return false;
	}

	return true;
}

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
		g_backBufferCount,
		a_fullscreen,
		a_verticalSyncInterval,
		g_swapchain))
	{
		Error("Failed to create swapchain!");
		return PUG_RESULT_GRAPHICS_ERROR;
	}

	g_currentBackBufferIndex = g_swapchain->GetCurrentBackBufferIndex();

	PUG_TRY(InitCommittedDescriptorHeap(
		g_device,
		128,
		8,
		8,
		128,
		128));

	MapBackBuffersToTextures(
		g_device,
		g_swapchain,
		g_backBuffers,
		g_backBufferCount,
		windowsWindow->GetSize()
	);

	for(uint32_t i = 0; i < g_backBufferCount; ++i)
	{
		if (FAILED(g_device->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(&g_commandAllocators[i]))))
		{
			Error("Failed to create command allocator for back buffer %d!", i);
		}
	}

	if (FAILED(g_device->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		g_commandAllocators[g_currentBackBufferIndex],
		nullptr,
		IID_PPV_ARGS(&g_commandList))))
	{
		Error("Failed to create command list!");
	}
	g_commandList->Close();

	g_device->CreateFence(1, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_fence));
	g_fenceValue = 1;
	g_fenceEvent = CreateEvent(nullptr, 0, FALSE, nullptr);;
		
	return PUG_RESULT_OK;
}

PUG_RESULT pug::assets::graphics::DestroyGraphics()
{
	DestroyCommittedDescriptorHeap();

	CloseHandle(g_fenceEvent);
	SafeRelease(g_fence);
	SafeRelease(g_device);
	SafeRelease(g_debugDevice);
	SafeRelease(g_debugInterface);

	return PUG_RESULT_OK;
}

PUG_RESULT pug::assets::graphics::Render()
{
	//reset allocator
	if (FAILED(g_commandAllocators[g_currentBackBufferIndex]->Reset()))
	{
		Error("Failed to reset command allocator for frame %d", g_currentBackBufferIndex);
		return PUG_RESULT_GRAPHICS_ERROR;
	}

	RecordCommandList();

	ID3D12CommandList* commandLists[] = { g_commandList };
	g_commandQueue->ExecuteCommandLists(1, commandLists);
	//g_commandQueue->Signal()

	if (FAILED(g_swapchain->Present(0, 0)))
	{
		Error("Failed to present swapchain!");
		return PUG_RESULT_GRAPHICS_ERROR;
	}

	//execute
	//present
	//signal
	
	//sync
	const uint32_t localFenceValue = g_fenceValue;
	g_fenceValue += 1;

	if (FAILED(g_commandQueue->Signal(g_fence, localFenceValue)))
	{
		Error("Failed to schedule signal fence!");
		return PUG_RESULT_GRAPHICS_ERROR;
	}

	g_fence->SetEventOnCompletion(localFenceValue, g_fenceEvent);

	uint32_t completedValue = g_fence->GetCompletedValue();
	if (localFenceValue > completedValue)
	{
		WaitForSingleObject(g_fenceEvent, INFINITE);
	}

	g_currentBackBufferIndex = g_swapchain->GetCurrentBackBufferIndex();

	return PUG_RESULT_OK;
}

PUG_RESULT pug::assets::graphics::CreateTexture2D()
{
	return PUG_RESULT_UNKNOWN;
	//sort the data in subresource,
	//create ID3D12Resource
	//allocate descriptors
	//allocate texture in slot
	//return texture handle
}