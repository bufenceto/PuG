#include "graphics.h"

#include "defines.h"
#include "pug_funcs.h"

#include "dx12_helper.h"
#include "persistent_descriptor_heap.h"
#include "dx12_shader_librarian.h"

#include "resource/dx12_texture2D.h"
#include "resource/dx12_vertex_buffer.h"
#include "resource/dx12_index_buffer.h"

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
#define MAX_VERTEX_BUFFERS 128
#define MAX_INDEX_BUFFERS 128

#define BACK_BUFFER_FORMAT DXGI_FORMAT_R8G8B8A8_UNORM
#define DEPTH_BUFFER_FORMAT DXGI_FORMAT_D32_FLOAT

static DX12Texture2D g_textures[MAX_TEXTURES];
static DX12VertexBuffer g_vertexBuffers[MAX_VERTEX_BUFFERS];
static DX12IndexBuffer g_indexBuffers[MAX_INDEX_BUFFERS];

static const uint32_t g_backBufferCount = 2;

static uint32_t g_currentBackBufferIndex;

static ID3D12Debug* g_debugInterface;
static ID3D12Device* g_device;
static ID3D12DebugDevice* g_debugDevice;

static ID3D12DebugCommandQueue* g_debugCommandQueue;
static ID3D12CommandQueue* g_commandQueue;

static ID3D12Fence* g_fence;
static uint64_t g_fenceValue;
static HANDLE g_fenceEvent;

static IDXGISwapChain3* g_swapchain;
static DX12Texture2D g_backBuffers[g_backBufferCount];
static DX12Texture2D g_depthBuffer;

static ID3D12CommandAllocator* g_commandAllocators[g_backBufferCount];
static ID3D12GraphicsCommandList* g_commandList;

static ID3D12RootSignature* g_rootSignature;
static ID3D12PipelineState* g_pso;

static Vector3 g_clearColor = RIGHT;

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
			
			PUG_TRY(AllocatePersistentRTVDescriptors(rtvDescriptorIndex));
			PUG_TRY(GetPersistentRTVDescriptors(rtvDescriptorIndex, cpuRTVHandle, gpuRTVHandle));
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
	PUG_TRY(GetPersistentRTVDescriptors(g_backBuffers[g_currentBackBufferIndex].GetRTVHeapIndex(), cpuRenderTargetRTV, gpuRenderTargetRTV));
	
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

PUG_RESULT CreateRootSignatures(
	ID3D12Device* const a_device)
{
	HRESULT result;

	D3D12_ROOT_DESCRIPTOR rootCBVDescriptorPerObject;
	rootCBVDescriptorPerObject.RegisterSpace = 0;
	rootCBVDescriptorPerObject.ShaderRegister = 0;

	D3D12_ROOT_DESCRIPTOR rootCBVDescriptorPointLights;
	rootCBVDescriptorPointLights.RegisterSpace = 0;
	rootCBVDescriptorPointLights.ShaderRegister = 1;

	D3D12_ROOT_DESCRIPTOR rootCBVDescriptorDirectionalLights;
	rootCBVDescriptorDirectionalLights.RegisterSpace = 0;
	rootCBVDescriptorDirectionalLights.ShaderRegister = 2;

	D3D12_DESCRIPTOR_RANGE descAlbedoRanges[] =
	{
		{ D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND },
	};

	D3D12_ROOT_DESCRIPTOR_TABLE albedoDescriptorTable;
	albedoDescriptorTable.NumDescriptorRanges = PUG_COUNT_OF(descAlbedoRanges);
	albedoDescriptorTable.pDescriptorRanges = &descAlbedoRanges[0];

	D3D12_DESCRIPTOR_RANGE descNormalRanges[] =
	{
		{ D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND },
	};

	D3D12_ROOT_DESCRIPTOR_TABLE normalDescriptorTable;
	normalDescriptorTable.NumDescriptorRanges = PUG_COUNT_OF(descNormalRanges);
	normalDescriptorTable.pDescriptorRanges = &descNormalRanges[0];

	D3D12_STATIC_SAMPLER_DESC anisotropicSampler = {};
	anisotropicSampler.Filter = D3D12_FILTER_MAXIMUM_ANISOTROPIC;
	anisotropicSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	anisotropicSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	anisotropicSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	anisotropicSampler.MipLODBias = 0;
	anisotropicSampler.MaxAnisotropy = 16;
	anisotropicSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	anisotropicSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
	anisotropicSampler.MinLOD = 0.0f;
	anisotropicSampler.MaxLOD = D3D12_FLOAT32_MAX;
	anisotropicSampler.ShaderRegister = 0;
	anisotropicSampler.RegisterSpace = 0;
	anisotropicSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_PARAMETER rootParams[5] = {};

	rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParams[0].Descriptor = rootCBVDescriptorPerObject;
	rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParams[1].Descriptor = rootCBVDescriptorPointLights;
	rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParams[2].Descriptor = rootCBVDescriptorDirectionalLights;
	rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	rootParams[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[3].DescriptorTable = albedoDescriptorTable;
	rootParams[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	rootParams[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[4].DescriptorTable = normalDescriptorTable;
	rootParams[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	//rootParams[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	//rootParams[3].DescriptorTable = albedoDescriptorTable;
	//rootParams[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	//
	//rootParams[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	//rootParams[4].DescriptorTable = normalDescriptorTable;
	//rootParams[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_STATIC_SAMPLER_DESC samplers[] = { anisotropicSampler };

	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.NumParameters = PUG_COUNT_OF(rootParams);
	rootSignatureDesc.pParameters = rootParams;
	rootSignatureDesc.NumStaticSamplers = PUG_COUNT_OF(samplers);
	rootSignatureDesc.pStaticSamplers = samplers;
	rootSignatureDesc.Flags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	ID3DBlob* signature = nullptr;
	ID3DBlob* error = nullptr;
	result = D3D12SerializeRootSignature(
		&rootSignatureDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		&signature,
		&error);
	if (FAILED(result))
	{
		Error("Failed to serialize the combine root signature");
		return false;
	}

	result = a_device->CreateRootSignature(
		0,
		signature->GetBufferPointer(),
		signature->GetBufferSize(),
		IID_PPV_ARGS(&g_rootSignature));
	if (FAILED(result))
	{
		Error("Failed to create the combine root signature");
		return false;
	}

	return PUG_RESULT_OK;
}

PUG_RESULT CreatePSOs(
	ID3D12Device* const a_device)
{
	HRESULT hr;

	D3D12_BLEND_DESC blendDesc = {};
	blendDesc.RenderTarget[0].BlendEnable = FALSE;
	blendDesc.RenderTarget[0].LogicOpEnable = FALSE;
	blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_CLEAR;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;// 0x0F;

	D3D12_RASTERIZER_DESC rasterizerDesc = {};
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
	rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
	rasterizerDesc.FrontCounterClockwise = false;
	rasterizerDesc.DepthBias = 0;
	rasterizerDesc.DepthBiasClamp = 0.0f;
	rasterizerDesc.DepthClipEnable = TRUE;
	rasterizerDesc.MultisampleEnable = FALSE;
	rasterizerDesc.AntialiasedLineEnable = FALSE;
	rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {};
	depthStencilDesc.DepthEnable = TRUE;
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	depthStencilDesc.StencilEnable = FALSE;
	depthStencilDesc.StencilReadMask = 0xFF;
	depthStencilDesc.StencilWriteMask = 0xFF;
	//depthStencilDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
	//depthStencilDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
	//depthStencilDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC::

	D3D12_INPUT_ELEMENT_DESC inputDescs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA , 0 },
		{ "UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	uint8_t* forwardVertexShaderByteCode = nullptr;
	uint64_t forwardVertexShaderByteCodeSize = 0;
	PUG_ASSERT(GetShader("forward_vertex_shader.vs.hlsl", forwardVertexShaderByteCode, forwardVertexShaderByteCodeSize), "Failed to get compiled gbuffer vertex shader data");

	uint8_t* forwardPixelShaderByteCode = nullptr;
	uint64_t forwardPixelShaderByteCodeSize = 0;
	PUG_ASSERT(GetShader("forward_pixel_shader.ps.hlsl", forwardPixelShaderByteCode, forwardPixelShaderByteCodeSize), "Failed to get compiled gbuffer pixel shader data");

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = g_rootSignature;
	psoDesc.VS = { forwardVertexShaderByteCode, forwardVertexShaderByteCodeSize };
	psoDesc.PS = { forwardPixelShaderByteCode, forwardPixelShaderByteCodeSize };
	psoDesc.BlendState = blendDesc;//CD3DX12_BLEND_DESC(D3D12_DEFAULT);// blendDesc;
	psoDesc.SampleMask = 1;
	psoDesc.RasterizerState = rasterizerDesc;//CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);// rasterizerDesc;
	psoDesc.DepthStencilState = depthStencilDesc;//CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);// depthStencilDesc;
	psoDesc.InputLayout = { inputDescs , PUG_COUNT_OF(inputDescs) };
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = BACK_BUFFER_FORMAT;
	psoDesc.DSVFormat = DEPTH_BUFFER_FORMAT;
	psoDesc.SampleDesc = { 1, 0 };
	psoDesc.NodeMask = 0;
	psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	hr = a_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&g_pso));
	if (FAILED(hr))
	{
		Error("Failed to create forward pipeline state!");
		return PUG_RESULT_GRAPHICS_ERROR;
	}

	return PUG_RESULT_OK;
}

PUG_RESULT CreateDepthBuffer(
	const Int2& windowSize)
{
	//the depth buffer is not a simple texture, custom initialisation is needed
	HRESULT hr;
	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = DXGI_FORMAT_D32_FLOAT;
	clearValue.Color[0] = 1.0f;

	D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_RESOURCE_DESC resDesc(D3D12_RESOURCE_DIMENSION_TEXTURE2D, 0,
		(UINT64)windowSize.x, (UINT64)windowSize.y, 1, 1,
		DEPTH_BUFFER_FORMAT, 1, 0, D3D12_TEXTURE_LAYOUT_UNKNOWN,
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

	ID3D12Resource* resource;
	hr = g_device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
		&clearValue,
		IID_PPV_ARGS(&resource));
	if (FAILED(hr))
	{
		Error("Failed to create deferred depth resource");
		return PUG_RESULT_FAILED_TO_CREATE_GPU_RESOURCE;
	}

	//DSV
	uint32_t dsvHeapIndex = 0;
	{
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = DEPTH_BUFFER_FORMAT;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

		D3D12_CPU_DESCRIPTOR_HANDLE CPUDSVHandle = {};
		D3D12_GPU_DESCRIPTOR_HANDLE GPUDSVHandle = {};
		if (!AllocatePersistentDSVDescriptors(dsvHeapIndex))
		{
			Error("Failed to allocate dsv descriptors for simple 2D texture");
			resource->Release();
			return false;
		}

		GetPersistentDSVDescriptors(dsvHeapIndex, CPUDSVHandle, GPUDSVHandle);
		g_device->CreateDepthStencilView(resource, &dsvDesc, CPUDSVHandle);
	}

	g_depthBuffer = DX12Texture2D(
		resource, 
		PUG_INVALID_ID, 
		dsvHeapIndex, 
		PUG_INVALID_ID, 
		PUG_INVALID_ID, 
		PUG_INVALID_ID, 
		windowSize.x,
		windowSize.y,
		DEPTH_BUFFER_FORMAT, 
		D3D12_RESOURCE_STATE_DEPTH_WRITE);
	SET_NAME(g_depthBuffer);

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

	PUG_TRY(InitPersistentDescriptorHeap(
		g_device,
		128,
		8,
		8,
		128,
		128));

	PUG_TRY(InitShaderLibrarian());

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

	PUG_TRY(CreateDepthBuffer(windowsWindow->GetSize()));
	PUG_TRY(CreateRootSignatures(g_device));
	PUG_TRY(CreatePSOs(g_device));

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
	DestroyPersistentDescriptorHeap();

	CloseHandle(g_fenceEvent);
	SafeRelease(g_fence);
	SafeRelease(g_commandList);
	SafeRelease(g_rootSignature);

	for (uint32_t i = 0; i < g_backBufferCount; ++i)
	{
		SafeRelease(g_commandAllocators[i]);
	}

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

	if (FAILED(g_swapchain->Present(0, 0)))
	{
		Error("Failed to present swapchain!");
		return PUG_RESULT_GRAPHICS_ERROR;
	}
	
	//sync
	const uint64_t localFenceValue = g_fenceValue;
	g_fenceValue += 1;

	if (FAILED(g_commandQueue->Signal(g_fence, localFenceValue)))
	{
		Error("Failed to schedule signal fence!");
		return PUG_RESULT_GRAPHICS_ERROR;
	}

	g_fence->SetEventOnCompletion(localFenceValue, g_fenceEvent);

	uint64_t completedValue = g_fence->GetCompletedValue();
	if (localFenceValue > completedValue)
	{
		WaitForSingleObject(g_fenceEvent, INFINITE);
	}

	g_currentBackBufferIndex = g_swapchain->GetCurrentBackBufferIndex();

	return PUG_RESULT_OK;
}

PUG_RESULT pug::assets::graphics::CreateVertexBuffer(
	const void* a_data,
	const uint64_t a_vertexStride,
	const uint32_t a_vertexCount,
	VertexBufferHandle& out_result)
{
	uint32_t index = PUG_INVALID_ID;
	for (uint32_t i = 0; i < MAX_VERTEX_BUFFERS; ++i)
	{
		if (i != PUG_INVALID_ID && (g_vertexBuffers[i].IsInitialized() == 0))
		{
			index = i;
			break;
		}
	}
	if (index == PUG_INVALID_ID)
	{//no room in vertex buffer array
		return PUG_RESULT_ALLOCATION_FAILURE;
	}

	HRESULT hr;
	ID3D12Resource* resource = nullptr;
	uint64_t dataSize = a_vertexStride * a_vertexCount;
	//D3D12_HEAP_PROPERTIES defaultHeapProps = CreatSimpleDefaultHeapProperties();
	//D3D12_RESOURCE_DESC resourceDesc = CreateSimpleBufferResourceDesc(dataSize);
	hr = g_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(dataSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&resource));
	if (FAILED(hr))
	{
		Error("Failed to create vertex buffer.");
		return PUG_RESULT_FAILED_TO_CREATE_GPU_RESOURCE;
	}

	if (a_data != nullptr)
	{
		UINT8* dest = nullptr;
		CD3DX12_RANGE readRange(0, 0);// We do not intend to read from this resource on the CPU.
		resource->Map(0, &readRange, (void**)&dest);
		memcpy(dest, a_data, dataSize);
		resource->Unmap(0, nullptr);
	}

	g_vertexBuffers[index] = DX12VertexBuffer(
		resource, 
		a_vertexStride,
		a_vertexCount, 
		D3D12_RESOURCE_STATE_GENERIC_READ);

	out_result = index;
	return PUG_RESULT_OK;
}

PUG_RESULT pug::assets::graphics::CreateIndexBuffer(
	const void* a_data,
	const PUG_FORMAT a_indexFormat,
	const uint32_t a_indexCount,
	IndexBufferHandle& out_result)
{
	uint32_t index = PUG_INVALID_ID;
	for (uint32_t i = 0; i < MAX_INDEX_BUFFERS; ++i)
	{
		if (i != PUG_INVALID_ID && (g_indexBuffers[i].IsInitialized() == 0))
		{
			index = i;
			break;
		}
	}
	if (index == PUG_INVALID_ID)
	{//no room in vertex buffer array
		return PUG_RESULT_ALLOCATION_FAILURE;
	}

	//Allocate memory for the vertex buffer on the GPU by creating a comitted resource
	HRESULT hr;
	ID3D12Resource* resource = nullptr;
	DXGI_FORMAT indexFormat = ConvertFormat(a_indexFormat);
	uint64_t dataSize = FormatToSize(indexFormat) * a_indexCount;
	hr = g_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(dataSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&resource));
	if (FAILED(hr))
	{
		Error("Failed to create index buffer.");
		return PUG_RESULT_FAILED_TO_CREATE_GPU_RESOURCE;
	}

	if (a_data != nullptr)
	{
		UINT8* dest = nullptr;
		CD3DX12_RANGE readRange(0, 0);// We do not intend to read from this resource on the CPU.
		resource->Map(0, &readRange, (void**)&dest);
		memcpy(dest, a_data, dataSize);
		resource->Unmap(0, nullptr);
	}

	g_indexBuffers[index] = DX12IndexBuffer(
		resource,
		indexFormat,
		a_indexCount,
		D3D12_RESOURCE_STATE_GENERIC_READ);

	out_result = index;
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