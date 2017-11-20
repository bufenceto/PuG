//main header
#include "graphics.h"

//pug dx12 headers
#include "dx12_helper.h"
#include "dx12_persistent_descriptor_heap.h"
#include "dx12_shader_librarian.h"
#include "dx12_constant_buffer.h"
#include "resource/dx12_texture2D.h"
#include "resource/dx12_vertex_buffer.h"
#include "resource/dx12_index_buffer.h"

//platform headers
#include "windows_window.h"

//pug headers
#include "macro.h"
#include "camera.h"
#include "mesh.h"
#include "transform.h"
#include "defines.h"
#include "pug_funcs.h"
//#include "PUG_material.h"
//#include "PUG_light.h"
//#include "PUG_dds.h"
//#include "PUG_rect.h"

//dx12 headers
#include <d3d12.h>
#include <directx12/d3dx12.h>
#include <dxgi1_4.h>

//std
#include <vector>
#include <atomic>

//imgui
#include "imgui/shaders/imgui_vertex_shader.h"
#include "imgui/shaders/imgui_pixel_shader.h"

using namespace pug;
using namespace pug::assets;
using namespace pug::assets::graphics;
using namespace pug::log;
using namespace pug::platform;
using namespace pug::windows;

using namespace vmath;

struct PerObjectBuffer
{
	vmath::Matrix4 worldMatrix;
	vmath::Matrix4 viewProjectionMatrix;
	vmath::Matrix4 inverseTransposeWorldMatrix;
	//----------------------------------------------- 192 bytes
	vmath::Vector3 cameraPosition;
	float padding1;
	//----------------------------------------------- 208
	vmath::Vector3 aabbColor;
	uint32_t drawCallIndex;
	//----------------------------------------------- 224
	uint32_t numDrawCalls;
	float padding2[3];
	//----------------------------------------------- 240
	float padding3[4];
	//----------------------------------------------- 256
};//64 floats = 256 bytes; perfect
static_assert(sizeof(PerObjectBuffer) % 256 == 0, "Invalid Size");

#define MAX_TEXTURES 512
#define MAX_VERTEX_BUFFERS 128
#define MAX_INDEX_BUFFERS 128

#define BACK_BUFFER_FORMAT DXGI_FORMAT_R8G8B8A8_UNORM
#define DEPTH_BUFFER_FORMAT DXGI_FORMAT_D32_FLOAT

#define DRAW_CALLS_PER_PAGE (DX12_RESOURCE_SINGLE_TEXTURE_OR_CBUFFER_ALIGNMENT / sizeof(PerObjectBuffer))

//dx12 
static const uint32_t g_backBufferCount = 2;

static ID3D12DebugDevice* g_debug_device;
static ID3D12Device* g_device;
static ID3D12Debug* g_debugInterface;

static ID3D12CommandQueue* g_commandQueue;
static ID3D12DebugCommandQueue* g_debug_commandQueue;

static ID3D12Fence* g_fence;
static std::atomic_uint64_t g_fenceValue;
static HANDLE g_fenceEvent;

static IDXGISwapChain3* g_swapChain;

static DX12Texture2D g_depthBuffers[g_backBufferCount];
static DX12Texture2D g_backBuffers[g_backBufferCount];
static ID3D12CommandAllocator* g_commandAllocator[g_backBufferCount];

static ID3D12RootSignature* g_rootSignature;
static ID3D12RootSignature* g_guiRootSignature;

static ID3D12DebugCommandList* g_debug_commandList;
static ID3D12GraphicsCommandList* g_commandList;

static ID3D12DebugCommandList* g_debug_resourceCommandList;
static ID3D12GraphicsCommandList* g_resourceCommandList;

static ID3D12DebugCommandList* g_debug_guiCommandList;
static ID3D12GraphicsCommandList* g_guiCommandList;

static ID3D12PipelineState* g_pso;
static ID3D12PipelineState* g_guiPSO;

static D3D12_CPU_DESCRIPTOR_HANDLE g_nullCPUSrvHandle;
static D3D12_GPU_DESCRIPTOR_HANDLE g_nullGPUSrvHandle;
static D3D12_CPU_DESCRIPTOR_HANDLE g_nullCPUDsvHandle;
static D3D12_GPU_DESCRIPTOR_HANDLE g_nullGPUDsvHandle;
static D3D12_CPU_DESCRIPTOR_HANDLE g_nullCPURtvHandle;
static D3D12_GPU_DESCRIPTOR_HANDLE g_nullGPURtvHandle;

static D3D12_VIEWPORT g_viewPort;
static D3D12_RECT g_scissorRect;

static Int2 g_windowSize;
static Int2 g_newWindowSize;
static uint32_t g_resizeScreen;

static uint32_t g_currentBackBufferIndex;

static DX12Texture2D g_textures[MAX_TEXTURES];
static DX12VertexBuffer g_vertexBuffers[MAX_VERTEX_BUFFERS];
static DX12IndexBuffer g_indexBuffers[MAX_INDEX_BUFFERS];

Vector3 g_clearColor = RIGHT;

static ConstantBufferHeap* g_constantBufferHeap;
static ConstantBufferHeap* g_guiConstantBufferHeap;

/*

//dds loader helper function
static void CalculateSurfaceInfo(
const size_t& width,
const size_t& height,
size_t* out_numBytes,
size_t* out_rowBytes)
{
size_t numBlocksWide = 0;
if (width > 0)
{
numBlocksWide = VMATH_MAX(1, (width + 3) / 4);
}
size_t numBlocksHigh = 0;
if (height > 0)
{
numBlocksHigh = VMATH_MAX(1, (height + 3) / 4);
}
*out_rowBytes = numBlocksWide * BC3_DDS_BPE;
*out_numBytes = *out_rowBytes * numBlocksHigh;
}

//dds loading helper function
static int32_t FillInitData(
const size_t width,
const size_t height,
const size_t depth,
const size_t mipCount,
const size_t arraySize,
const size_t maxsize,
const size_t bitSize,
const uint8_t* bitData,
D3D12_SUBRESOURCE_DATA* initData)
{
if (!bitData || !initData)
{
return -1;
}

size_t skipMip = 0;
size_t twidth = 0;
size_t theight = 0;
size_t tdepth = 0;

size_t numBytes = 0;
size_t rowBytes = 0;
const uint8_t* pSrcBits = bitData;
const uint8_t* pEndBits = bitData + bitSize;

size_t index = 0;
for (size_t j = 0; j < arraySize; j++)
{
size_t w = width;
size_t h = height;
size_t d = depth;
for (size_t i = 0; i < mipCount; i++)
{
CalculateSurfaceInfo(w, h, &numBytes, &rowBytes);

if ((mipCount <= 1) || !maxsize || (w <= maxsize && h <= maxsize && d <= maxsize))
{
if (!twidth)
{
twidth = w;
theight = h;
tdepth = d;
}

PUG_ASSERT(index < mipCount * arraySize, "Out of bounds!");
initData[index].pData = (const void*)pSrcBits;
initData[index].RowPitch = static_cast<UINT>(rowBytes);
initData[index].SlicePitch = static_cast<UINT>(numBytes);
++index;
}
else if (!j)
{
// Count number of skipped mipmaps (first item only)
++skipMip;
}

if (pSrcBits + (numBytes*d) > pEndBits)
{
return -1;
}

pSrcBits += numBytes * d;

w = w >> 1;
h = h >> 1;
d = d >> 1;
if (w == 0)
{
w = 1;
}
if (h == 0)
{
h = 1;
}
if (d == 0)
{
d = 1;
}
}
}

return (index > 0) ? 0 : -1;
}

*/

static void DX12DestroyVertexBuffer(
	DX12VertexBuffer& vb)
{
	uint64_t completedValue = g_fence->GetCompletedValue();
	if (completedValue + 1 < g_fenceValue)
	{
		WaitForSingleObject(g_fenceEvent, INFINITE);
	}

	if (vb.IsInitialized())
	{
		ID3D12Resource* resource = vb.GetResource();
		SafeRelease(resource);
		vb.~DX12VertexBuffer();
	}
}

static void DX12DestroyIndexBuffer(
	DX12IndexBuffer& ib)
{
	uint64_t completedValue = g_fence->GetCompletedValue();
	if (completedValue + 1 < g_fenceValue)
	{
		WaitForSingleObject(g_fenceEvent, INFINITE);
	}

	if (ib.IsInitialized())
	{
		ID3D12Resource* resource = ib.GetResource();
		SafeRelease(resource);
		ib.~DX12IndexBuffer();
	}
}

static void DX12DestroyTexture2D(
	DX12Texture2D& texture)
{
	uint64_t completedValue = g_fence->GetCompletedValue();
	if (completedValue + 1 < g_fenceValue)
	{
		WaitForSingleObject(g_fenceEvent, INFINITE);
	}

	if (texture.IsInitialized())
	{
		ReleasePersistentSRVDescriptors(texture.GetSRVHeapIndex());
		ID3D12Resource* resource = texture.GetResource();
		SafeRelease(resource);
		texture.~DX12Texture2D();
	}
}

static PUG_RESULT GetBackbuffers(
	ID3D12Device* a_device)
{
	PUG_ASSERT(a_device, "Device is not initialized!");

	HRESULT result;

	for (uint32_t i = 0; i < g_backBufferCount; ++i)
	{
		ID3D12Resource* backBuffer = nullptr;
		result = g_swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer));//write the pointer of our back buffer to our textures
		if (FAILED(result))
		{
			Error("Failed to get pointer to back buffer in swap chain with index: %d!", i);
			return PUG_RESULT_GRAPHICS_ERROR;
		}
		//SRV
		{
			D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = {};
			D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = {};
			//D3D12_CPU_DESCRIPTOR_HANDLE renderTargetViewHandle = {};
			uint32_t rtvDescriptorIndex = AllocatePersistentRTVDescriptors();
			PUG_ASSERT(rtvDescriptorIndex != PUG_INVALID_ID, "Failed to allocate RTV Descriptor");

			PUG_TRY(GetPersistentRTVDescriptors(rtvDescriptorIndex, &cpuHandle, nullptr));
			a_device->CreateRenderTargetView(backBuffer, NULL, cpuHandle);

			//Datatype *x = new(y) Datatype();

			new (&g_backBuffers[i]) DX12Texture2D(
										backBuffer,
										0,
										0,
										rtvDescriptorIndex,
										0,
										0,
										g_windowSize.x,
										g_windowSize.y,
										BACK_BUFFER_FORMAT,
										D3D12_RESOURCE_STATE_PRESENT);
		}
	}

	return PUG_RESULT_OK;
}

static PUG_RESULT CreateDepthBuffers(
	uint32_t a_screenWidth,
	uint32_t a_screenHeight,
	DXGI_FORMAT a_depthBufferFormat)
{
	HRESULT hr;
	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = DXGI_FORMAT_D32_FLOAT;
	clearValue.Color[0] = 1.0f;

	for(uint32_t i = 0; i < g_backBufferCount; ++i)
	{
		ID3D12Resource* resource = nullptr;

		D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		CD3DX12_RESOURCE_DESC resDesc(D3D12_RESOURCE_DIMENSION_TEXTURE2D, 0,
			(UINT64)a_screenWidth, (UINT64)a_screenHeight, 1, 1,
			a_depthBufferFormat, 1, 0, D3D12_TEXTURE_LAYOUT_UNKNOWN,
			D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

		hr = g_device->CreateCommittedResource(&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&clearValue,
			IID_PPV_ARGS(&resource));
		if (FAILED(hr))
		{
			Error("Failed to create deferred depth resource");
			return PUG_RESULT_FAILED_TO_CREATE_GPU_RESOURCE;
		}

		//DSV
		{
			D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
			dsvDesc.Format = a_depthBufferFormat;
			dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

			uint32_t dsvHeapIndex = AllocatePersistentDSVDescriptors();
			PUG_ASSERT(dsvHeapIndex != PUG_INVALID_ID, "Failed to allocate descriptors for back buffer!");

			D3D12_CPU_DESCRIPTOR_HANDLE CPUDSVHandle = {};
			PUG_TRY(GetPersistentDSVDescriptors(dsvHeapIndex, &CPUDSVHandle, nullptr));
			g_device->CreateDepthStencilView(resource, &dsvDesc, CPUDSVHandle);

			new (&g_depthBuffers[i]) DX12Texture2D(
				resource,
				PUG_INVALID_ID,
				dsvHeapIndex,
				PUG_INVALID_ID,
				PUG_INVALID_ID,
				PUG_INVALID_ID,
				a_screenWidth,
				a_screenHeight,
				a_depthBufferFormat,
				D3D12_RESOURCE_STATE_DEPTH_WRITE);
		}
	}
	return PUG_RESULT_OK;
}

static bool CreatePersistentDescriptorHeap(
	ID3D12Device* a_device)
{
	PUG_ASSERT(a_device, "Device is not initialized!");

	PUG_TRY(InitPersistentDescriptorHeap(
		a_device,
		128,
		128,
		128,
		128,
		128));

	//null descriptors, used for telling the pipeline that nothing is bound
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC nullSrvDesc = {};
		nullSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		nullSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		nullSrvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		nullSrvDesc.Texture2D.MipLevels = 1;
		nullSrvDesc.Texture2D.MostDetailedMip = 0;
		nullSrvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

		uint32_t nullSRVIndex = AllocatePersistentSRVDescriptors();
		PUG_ASSERT(nullSRVIndex != PUG_INVALID_ID, "failed to allocate null srv descriptors!");
		PUG_TRY(GetPersistentSRVDescriptors(nullSRVIndex, &g_nullCPUSrvHandle, &g_nullGPUSrvHandle))
		a_device->CreateShaderResourceView(nullptr, &nullSrvDesc, g_nullCPUSrvHandle);

		D3D12_DEPTH_STENCIL_VIEW_DESC nullDsvDesc = {};
		nullDsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		nullDsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
		nullDsvDesc.Texture2D.MipSlice = 0;

		uint32_t nullDSVIndex = AllocatePersistentDSVDescriptors();
		PUG_ASSERT(nullDSVIndex != PUG_INVALID_ID, "failed to allocate null dsv descriptors!");
		PUG_TRY(GetPersistentDSVDescriptors(nullDSVIndex, &g_nullCPUDsvHandle, &g_nullGPUDsvHandle));
		a_device->CreateDepthStencilView(nullptr, &nullDsvDesc, g_nullCPUDsvHandle);

		D3D12_RENDER_TARGET_VIEW_DESC nullRtvDesc = {};
		nullRtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		nullRtvDesc.Format = BACK_BUFFER_FORMAT;
		nullRtvDesc.Texture2D.MipSlice = 0;
		nullRtvDesc.Texture2D.PlaneSlice = 0;

		uint32_t nullRTVIndex = AllocatePersistentRTVDescriptors();
		PUG_ASSERT(nullRTVIndex != PUG_INVALID_ID, "failed to allocate null rtv descriptors!");
		PUG_TRY(GetPersistentRTVDescriptors(nullRTVIndex, &g_nullCPURtvHandle, &g_nullGPURtvHandle));
		a_device->CreateRenderTargetView(nullptr, &nullRtvDesc, g_nullCPURtvHandle);
	}
	return true;
}

static bool CreateCommandAllocators(
	ID3D12Device* device)
{
	PUG_ASSERT(device, "Device is not initialized!");

	HRESULT result;
	// Create a command allocator.
	for (uint32_t i = 0; i < g_backBufferCount; ++i)
	{
		//result = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS(&g_computeCommandAllocator[i]));
		//if (FAILED(result))
		//{
		//	Error("Failed to create compute command allocator for backbuffer %d", i);
		//	return false;
		//}
		//DX12_SET_NAME(g_computeCommandAllocator[i]);

		result = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_commandAllocator[i]));
		if (FAILED(result))
		{
			Error("Failed to create command allocator for backbuffer %d", i);
			return false;
		}
		DX12_SET_NAME(g_commandAllocator[i]);
	}

	return true;
}

static bool CreateCommandList(
	ID3D12Device* a_device)
{
	PUG_ASSERT(a_device, "Device is not initialized!");

	HRESULT result;

	//command list
	{
		result = a_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_commandAllocator[g_currentBackBufferIndex], NULL, IID_PPV_ARGS(&g_commandList));
		if (FAILED(result))
		{
			Error("Failed to create direct command list!");
			return false;
		}
		result = g_commandList->Close();
		if (FAILED(result))
		{
			Error("Failed to close direct command list!");
			return false;
		}
		DX12_SET_NAME(g_commandList);//->SetName(L"g_lpvPropagateCommandList");
									 //Initially we need to reset the command list during initialization as it is created in a recording state.
	}
	//resource command list
	{
		result = a_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_commandAllocator[g_currentBackBufferIndex], NULL, IID_PPV_ARGS(&g_resourceCommandList));
		if (FAILED(result))
		{
			Error("Failed to create resource command list!");
			return false;
		}
		result = g_resourceCommandList->Close();
		if (FAILED(result))
		{
			Error("Failed to close g_resourceCommandList command list!");
			return false;
		}
		DX12_SET_NAME(g_resourceCommandList);
	}
	//imgui command list
	{
		result = a_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_commandAllocator[g_currentBackBufferIndex], NULL, IID_PPV_ARGS(&g_guiCommandList));
		if (FAILED(result))
		{
			Error("Failed to create resource command list!");
			return false;
		}
		result = g_guiCommandList->Close();
		if (FAILED(result))
		{
			Error("Failed to close g_resourceCommandList command list!");
			return false;
		}
		DX12_SET_NAME(g_guiCommandList);
	}

	return true;
}

static bool CreateRootSignatures(
	ID3D12Device* a_device)
{
	PUG_ASSERT(a_device, "Device is not initialized!");

	HRESULT result;
	//copy ray trace result buffer to back buffer root sig
	{
		D3D12_ROOT_DESCRIPTOR rootCBVDescriptorPerObject;
		rootCBVDescriptorPerObject.RegisterSpace = 0;
		rootCBVDescriptorPerObject.ShaderRegister = 0;

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

		D3D12_ROOT_PARAMETER rootParams[1] = {};

		rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParams[0].Descriptor = rootCBVDescriptorPerObject;
		rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

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
		rootSignatureDesc.NumStaticSamplers = 0;// PUG_COUNT_OF(samplers);
		rootSignatureDesc.pStaticSamplers = nullptr;// samplers;
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
		DX12_SET_NAME(g_rootSignature);
	}
	//imgui root signature
	{
		D3D12_ROOT_DESCRIPTOR rootCBVDescriptorImGuiProjectioMatrix;
		rootCBVDescriptorImGuiProjectioMatrix.RegisterSpace = 0;
		rootCBVDescriptorImGuiProjectioMatrix.ShaderRegister = 0;

		D3D12_DESCRIPTOR_RANGE descImGuiRanges[] =
		{
			{ D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND },
		};

		D3D12_ROOT_DESCRIPTOR_TABLE imGuiDescriptorTable;
		imGuiDescriptorTable.NumDescriptorRanges = PUG_COUNT_OF(descImGuiRanges);
		imGuiDescriptorTable.pDescriptorRanges = &descImGuiRanges[0];

		D3D12_STATIC_SAMPLER_DESC sampler = {};
		sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.MipLODBias = 0;
		sampler.MaxAnisotropy = 0;
		sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
		sampler.MinLOD = 0.0f;
		sampler.MaxLOD = 0.0f;
		sampler.ShaderRegister = 0;
		sampler.RegisterSpace = 0;
		sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		D3D12_ROOT_PARAMETER rootParams[2] = {};
		rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParams[0].Descriptor = rootCBVDescriptorImGuiProjectioMatrix;
		rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

		rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParams[1].DescriptorTable = imGuiDescriptorTable;
		rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
		rootSignatureDesc.NumParameters = PUG_COUNT_OF(rootParams);
		rootSignatureDesc.pParameters = rootParams;
		rootSignatureDesc.NumStaticSamplers = 1;
		rootSignatureDesc.pStaticSamplers = &sampler;
		rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

		ID3DBlob* signature = nullptr;
		ID3DBlob* error = nullptr;
		result = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
		if (FAILED(result))
		{
			Error("Failed to serialize the gui root signature");
			return false;
		}

		result = a_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
			IID_PPV_ARGS(&g_guiRootSignature));
		if (FAILED(result))
		{
			Error("Failed to create ImGui root signature");
			return false;
		}
		DX12_SET_NAME(g_guiRootSignature);
	}

	return PUG_RESULT_OK;
}

static PUG_RESULT CreatePipelineStateObjects(
	ID3D12Device* a_device)
{
	HRESULT result;
	//cpso
	{
		//D3D12_COMPUTE_PIPELINE_STATE_DESC cpsoDesc = {};
		//cpsoDesc.pRootSignature = g_rootSignature;
		//cpsoDesc.CS = {};
		//cpsoDesc.NodeMask = 0;
		//cpsoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
		//
		//result = device->CreateComputePipelineState(&cpsoDesc, IID_PPV_ARGS(&g_cpso));
		//if (FAILED(result))
		//{
		//	Error("Failed to create compute pipeline state!");
		//	return false;
		//}
		//SET_NAME(g_cpso);
	}
	//pso
	{
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

		result = a_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&g_pso));
		if (FAILED(result))
		{
			Error("Failed to create forward pipeline state!");
			return PUG_RESULT_GRAPHICS_ERROR;
		}
		DX12_SET_NAME(g_pso);
	}
	//imgui pso
	{
		D3D12_BLEND_DESC blendDesc = {};
		blendDesc.RenderTarget[0].BlendEnable = TRUE;
		blendDesc.RenderTarget[0].LogicOpEnable = FALSE;
		blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;// 0x0F;

		D3D12_RASTERIZER_DESC rasterizerDesc = {};
		rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
		rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
		rasterizerDesc.FrontCounterClockwise = false;
		rasterizerDesc.DepthBias = 0;
		rasterizerDesc.DepthBiasClamp = 0.0f;
		rasterizerDesc.DepthClipEnable = FALSE;
		rasterizerDesc.MultisampleEnable = FALSE;
		rasterizerDesc.AntialiasedLineEnable = FALSE;
		rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

		D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {};
		depthStencilDesc.DepthEnable = FALSE;
		depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		depthStencilDesc.StencilEnable = FALSE;
		depthStencilDesc.StencilReadMask = 0xFF;
		depthStencilDesc.StencilWriteMask = 0xFF;

		D3D12_INPUT_ELEMENT_DESC inputDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,   0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,  0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,   0, 8,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,  0 },
			{ "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA , 0 },
		};

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = g_guiRootSignature;
		psoDesc.VS = { g_imguiVertexShader, sizeof(g_imguiVertexShader) };
		psoDesc.PS = { g_imguiPixelShader, sizeof(g_imguiPixelShader) };
		psoDesc.BlendState = blendDesc;//CD3DX12_BLEND_DESC(D3D12_DEFAULT);// blendDesc;
		psoDesc.SampleMask = 1;
		psoDesc.RasterizerState = rasterizerDesc;//CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);// rasterizerDesc;
		psoDesc.DepthStencilState = depthStencilDesc;//CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);// depthStencilDesc;
		psoDesc.InputLayout = { inputDescs , PUG_COUNT_OF(inputDescs) };
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.DSVFormat = DEPTH_BUFFER_FORMAT;
		psoDesc.SampleDesc = { 1, 0 };
		psoDesc.NodeMask = 0;
		psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

		result = a_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&g_guiPSO));
		if (FAILED(result))
		{
			Error("Failed to create imgui pipeline state!");
			return false;
		}
	}

	return true;
}

static bool RecreateWindowObjects(
	const vmath::Int2& newWindowSize)
{
	//wait for frame to finish rendering
	//WaitForFrameToComplete();

	PUG_ASSERT(false, "ADD RECREATION OF UPLOAD HEAP");

	HRESULT hr;

	//Destroy swap chain and recreate
	if (g_swapChain != nullptr)
	{
		for (uint32_t i = 0; i < PUG_COUNT_OF(g_backBuffers); ++i)
		{
			log::Error("Implement me!");
			//DX12DestroyTexture2D(
			//	&g_backBuffers[i],
			//	g_srvCbvUavHeap,
			//	g_rtvHeap,
			//	g_dsvHeap);
		}

		hr = g_swapChain->ResizeBuffers(0, newWindowSize.x, newWindowSize.y, BACK_BUFFER_FORMAT, 0);
		if (FAILED(hr))
		{
			return false;
		}
		if (!GetBackbuffers(g_device))
		{
			return false;
		}
	}

	g_currentBackBufferIndex = g_swapChain->GetCurrentBackBufferIndex();
	g_windowSize = newWindowSize;
	g_viewPort = { 0.0f, 0.0f, (float)newWindowSize.x, (float)newWindowSize.y, 0.0f, 1.0f };
	g_scissorRect = { 0l, 0l, (LONG)newWindowSize.x, (LONG)newWindowSize.y };

	return true;
}

/*
PUG_RESULT CreateConstantBufferPageHeap(
	ID3D12Device* a_device,
	const size_t a_heapSize_bytes,
	D3D12_HEAP_FLAGS a_heapFlags)
{
	//allocate heap
	D3D12_HEAP_DESC heapDesc =
		CD3DX12_HEAP_DESC::CD3DX12_HEAP_DESC(
			a_heapSize_bytes,
			D3D12_HEAP_TYPE_UPLOAD,
			KB(64),
			a_heapFlags);

	//heapDesc.SizeInBytes = a_heapSize_bytes;
	//heapDesc.Properties.Type = D3D12_HEAP_TYPE_UPLOAD;
	//heapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	//heapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	//heapDesc.Properties.CreationNodeMask = 1;///<TODO> what is this?
	//heapDesc.Properties.VisibleNodeMask = 1;///<TODO> what is this?
	//heapDesc.Alignment = KB(64);
	//heapDesc.Flags = a_heapFlags;

	if (FAILED(a_device->CreateHeap(&heapDesc, IID_PPV_ARGS(&g_constantBufferPagesHeap))))
	{
		Error("Failed to create constant buffer heap");
		return PUG_RESULT_FAILED_TO_CREATE_GPU_RESOURCE;
	}

	const size_t pageSize = KB(64);
	PUG_ASSERT(a_heapSize_bytes % pageSize == 0, "a_heapSize_bytes is not 64KB aligned!");

	const uint32_t pagesPerHeap = (uint32_t)a_heapSize_bytes / pageSize;
	g_placedConstantBufferPages = (ID3D12Resource**)_aligned_malloc(sizeof(ID3D12Resource*) * pagesPerHeap, 16);
	if (g_placedConstantBufferPages == nullptr)
	{
		return PUG_RESULT_ALLOCATION_FAILURE;
	}
	for (uint32_t i = 0; i < pagesPerHeap; ++i)
	{
		HRESULT res = S_FALSE;
		D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(pageSize);

		res = a_device->CreatePlacedResource(
			g_constantBufferPagesHeap,
			pageSize * i,
			&resDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&g_placedConstantBufferPages[i]));

		if (FAILED(res))
		{
			Error("Failed to create placed resource at index %d", i);
			SafeRelease(g_constantBufferPagesHeap);
			return PUG_RESULT_ALLOCATION_FAILURE;
		}
	}

	g_numPlacedConstantBufferPages = pagesPerHeap;
	return PUG_RESULT_OK;
}
*/
static PUG_RESULT PrepareFrameData(
	const Matrix4& viewMatrix,
	const Matrix4& projectionMatrix,
	const Vector3& cameraPosition,
	const Transform* a_transforms,
	const Mesh* a_meshes,
	const uint32_t a_numMeshes,
	size_t& out_meshCBufferBaseOffset,
	size_t& out_guiCBufferBaseOffset)
{
	//HRESULT hr;
	//mesh cbuffers
	{
		PUG_ALIGN(16) PerObjectBuffer perObjectData[DRAW_CALLS_PER_PAGE];

		const uint32_t numPagesNeeded = a_numMeshes % DRAW_CALLS_PER_PAGE;
		PUG_ASSERT(numPagesNeeded <= 1, "No Support for multi page rendering, yet!");

		for (uint32_t i = 0; i < a_numMeshes; ++i)
		{
			if (a_meshes[i].m_vbh == PUG_INVALID_ID || a_meshes[i].m_ibh == PUG_INVALID_ID)
			{//skip meshes with invalid handles
				continue;
			}

			//per object data
			Matrix4 worldMatrix = GetWorldMatrix(a_transforms[i]);

			perObjectData[i] =
			{
				worldMatrix,						//world matrix
				viewMatrix * projectionMatrix,		//vp matrix; might consider making this the mvp matrix
				Inverse(Transpose(worldMatrix)),	//inverse transpose world matrix for normal mapping
				cameraPosition,						//camera position
				0.0f,
				Vector3(),							//aabb color (NOT USED)
				i,									//draw call index (NOT USED)
				a_numMeshes,						//num draw calls
													//28 bytes padding
			};
		}

		//write to per object buffer
		g_constantBufferHeap->Write(&perObjectData, sizeof(PerObjectBuffer) * a_numMeshes, out_meshCBufferBaseOffset);
	}
	//gui cbuffer
	{
		Matrix4 ortho = utility::CreateOffCenterOrthographicProjectionMatrix(0.0f, (float)g_windowSize.x, 0.0f, (float)g_windowSize.y, 0.0f, 4.0f);
		g_guiConstantBufferHeap->Write(&ortho, sizeof(ortho), out_guiCBufferBaseOffset);
		/*
		uint8_t* guiDest = nullptr;
		hr = g_guiConstantBuffer.buffer->Map(0, &readRange, (void**)&guiDest);
		if (FAILED(hr))
		{
			Error("Failed to map gui constant buffer");
			return false;
		}
		memcpy(guiDest, &ortho, sizeof(ortho));
		g_guiConstantBuffer.buffer->Unmap(0, nullptr);
		*/
	}

	return PUG_RESULT_OK;
}

static bool RecordCommandList(
	const pug::assets::graphics::Mesh* const a_meshes,
	const uint32_t a_meshCount,
	const size_t meshCBufferBaseOffset)
{
	HRESULT result;

	result = g_commandList->Reset(g_commandAllocator[g_currentBackBufferIndex], g_pso);
	if (FAILED(result))
	{
		Error("Failed to reset command list for recording");
		return false;
	}

	D3D12_RESOURCE_BARRIER barriersIn[] =
	{
		//CD3DX12_RESOURCE_BARRIER::Transition(g_backBuffers[g_frameIndex].resource, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET),
		g_backBuffers[g_currentBackBufferIndex].Transition(D3D12_RESOURCE_STATE_RENDER_TARGET),
	};

	g_commandList->ResourceBarrier(PUG_COUNT_OF(barriersIn), barriersIn);

	D3D12_CPU_DESCRIPTOR_HANDLE renderTargetRTV = {};
	PUG_TRY(GetPersistentRTVDescriptors(g_backBuffers[g_currentBackBufferIndex].GetRTVHeapIndex(), &renderTargetRTV, nullptr));

	D3D12_CPU_DESCRIPTOR_HANDLE cpudepthBufferDSV = {};
	PUG_TRY(GetPersistentDSVDescriptors(g_depthBuffers[g_currentBackBufferIndex].GetDSVHeapIndex(), &cpudepthBufferDSV, nullptr));
	//g_commandList->OMSetRenderTargets(1, &cpuRenderTargetRTV, FALSE, &g_nullCPUDsvHandle);

	g_commandList->OMSetRenderTargets(1, &renderTargetRTV, FALSE, &cpudepthBufferDSV);

	g_clearColor = g_clearColor * Matrix3(Vector3(RADIANS(45.0f * GetDeltaTime())));

	const float clearColor[] = { g_clearColor.x, g_clearColor.y, g_clearColor.z, 0.0f };
	g_commandList->ClearRenderTargetView(renderTargetRTV, clearColor, 0, nullptr);
	g_commandList->ClearDepthStencilView(cpudepthBufferDSV, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	g_commandList->SetGraphicsRootSignature(g_rootSignature);

	g_commandList->RSSetViewports(1, &g_viewPort);
	g_commandList->RSSetScissorRects(1, &g_scissorRect);
	g_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	for (uint32_t i = 0; i < a_meshCount; ++i)
	{
		DX12VertexBuffer& vbuffer = g_vertexBuffers[a_meshes[i].m_vbh];
		DX12IndexBuffer& ibuffer = g_indexBuffers[a_meshes[i].m_ibh];

		g_commandList->IASetVertexBuffers(0, 1, &vbuffer.GetView());
		g_commandList->IASetIndexBuffer(&ibuffer.GetView());
		//the constant buffers will align with the meshes, 
		//this means that for mesh index i, the constant buffer with index i will hold that meshes world matrix etc.
		uint64_t increment = DX12_CONSTANT_BUFFER_ELEMENT_SIZE(sizeof(PerObjectBuffer));
		//g_commandList->SetGraphicsRootConstantBufferView(0, g_placedConstantBufferPages[0]->GetGPUVirtualAddress() + (i * increment));
		D3D12_GPU_VIRTUAL_ADDRESS cbufferAddr = g_constantBufferHeap->GetGPUVirtualAddress() + meshCBufferBaseOffset;
		g_commandList->SetGraphicsRootConstantBufferView(0, cbufferAddr + (i * increment));

		uint32_t numIndices = ibuffer.GetIndexCount();
		g_commandList->DrawIndexedInstanced(numIndices, 1, 0, 0, 0);
	}

	D3D12_RESOURCE_BARRIER barriersOut[] =
	{
		//CD3DX12_RESOURCE_BARRIER::Transition(g_backBuffers[g_frameIndex].resource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT),
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


static bool RecordGUICommandList(
const Mesh* guiMeshes,
const Rect* guiScissorRects,
const Texture2DHandle* guiTextures,
const uint32_t guiMeshCount,
const size_t guiCBufferBaseOffset)
{
	HRESULT result;
	// However, when ExecuteCommandList() is called on a particular command
	// list, that command list can then be reset at any time and must be before
	// re-recording

	result = g_guiCommandList->Reset(g_commandAllocator[g_currentBackBufferIndex], g_guiPSO);
	if (FAILED(result))
	{
		Error("Failed to reset gui command list!");
		return false;
	}

	//g_guiCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(g_backBuffers[g_currentBackBufferIndex].resource, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
	g_guiCommandList->ResourceBarrier(1, &g_backBuffers[g_currentBackBufferIndex].Transition(D3D12_RESOURCE_STATE_RENDER_TARGET));
	
	D3D12_CPU_DESCRIPTOR_HANDLE renderTarget = {};
	GetPersistentRTVDescriptors(g_backBuffers[g_currentBackBufferIndex].GetRTVHeapIndex(), &renderTarget, nullptr);
	g_guiCommandList->OMSetRenderTargets(1, &renderTarget, FALSE, &g_nullCPUDsvHandle);
	g_guiCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	
	g_guiCommandList->SetGraphicsRootSignature(g_guiRootSignature);

	D3D12_VIEWPORT guiViewPort = g_viewPort;
	D3D12_RECT scissorRect = g_scissorRect;

	g_guiCommandList->RSSetViewports(1, &guiViewPort);
	g_guiCommandList->RSSetScissorRects(1, &scissorRect);

	D3D12_GPU_VIRTUAL_ADDRESS cbufferAddr = g_guiConstantBufferHeap->GetGPUVirtualAddress() + guiCBufferBaseOffset;
	g_guiCommandList->SetGraphicsRootConstantBufferView(0, cbufferAddr);

	for (uint32_t i = 0; i < guiMeshCount; ++i)
	{
		VertexBufferHandle vertexBufferIndex = guiMeshes[i].m_vbh;
		IndexBufferHandle indexBufferIndex = guiMeshes[i].m_ibh;
		Texture2DHandle textureIndex = guiTextures[i];

		ID3D12DescriptorHeap* const heap = GetPersistentSRVDescriptorHeap(g_textures[textureIndex].GetSRVHeapIndex());
		g_guiCommandList->SetDescriptorHeaps(1, &heap);

		D3D12_GPU_DESCRIPTOR_HANDLE texGPUDescriptor = {};
		
		
		//gui::
		GetPersistentSRVDescriptors(g_textures[textureIndex].GetSRVHeapIndex(), nullptr, &texGPUDescriptor);
		
		g_guiCommandList->SetGraphicsRootDescriptorTable(1, texGPUDescriptor);
		g_guiCommandList->IASetIndexBuffer(&g_indexBuffers[indexBufferIndex].GetView());
		g_guiCommandList->IASetVertexBuffers(0, 1, &g_vertexBuffers[vertexBufferIndex].GetView());

		D3D12_RECT sr = {};
		sr.left = guiScissorRects[i].leftTop.x;
		sr.top = guiScissorRects[i].leftTop.y;
		sr.right = guiScissorRects[i].rightBottom.x;
		sr.bottom = guiScissorRects[i].rightBottom.y;

		g_guiCommandList->RSSetScissorRects(1, &sr);

		const uint32_t indexCount = g_indexBuffers[indexBufferIndex].GetIndexCount();
		g_guiCommandList->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);
	}

	//g_guiCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(g_backBuffers[g_currentBackBufferIndex].resource, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	g_guiCommandList->ResourceBarrier(1, &g_backBuffers[g_currentBackBufferIndex].Transition(D3D12_RESOURCE_STATE_PRESENT));

	g_guiCommandList->Close();

	return true;
}


PUG_RESULT pug::assets::graphics::InitGraphics(
	pug::platform::Window* a_window,
	uint32_t a_verticalSyncInterval,
	uint32_t a_fullscreen)
{
	PUG_ASSERT(a_window, "Window is not initialized!");

	Info("Initializing DX12...");
	WindowsWindow* windowsWindow = dynamic_cast<WindowsWindow*>(a_window);
	if (windowsWindow == nullptr)
	{
		Error("We can only create a dx12 device with a Windows window!");
		return PUG_RESULT_INVALID_ARGUMENTS;
	}

	g_windowSize = windowsWindow->GetSize();
	g_viewPort = { 0.0f, 0.0f, (float)g_windowSize.x, (float)g_windowSize.y, 0.0f, 1.0f };
	g_scissorRect = { 0l, 0l, (LONG)g_windowSize.x, (LONG)g_windowSize.y };

	//if we are debugging we want an active debug layer
	HRESULT result;
#ifdef _DEBUG// || defined(DX12_DEBUG)
	result = D3D12GetDebugInterface(IID_PPV_ARGS(&g_debugInterface));
	if (FAILED(result))
	{
		Error("Failed to get D3D12 Debug interface.");
		return PUG_RESULT_GRAPHICS_ERROR;
	}
	g_debugInterface->EnableDebugLayer();
#endif

	//the feature levels we want to try
	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};

	Info("Creating graphics device...");
	IDXGIFactory1* factory = nullptr;
	//result = CreateDXGIFactory2(0, __uuidof(IDXGIFactory4), (void**)&factory);
	result = CreateDXGIFactory1(__uuidof(IDXGIFactory4), (void**)&factory);
	if (FAILED(result))
	{
		Error("Failed to create DXGI factory!");
		return PUG_RESULT_GRAPHICS_ERROR;
	}

	IDXGIAdapter1* chosenAdapter = nullptr;
	if (!FindAdapter(factory, chosenAdapter))
	{
		Error("Failed to find a suitable hardware adapter");
		return false;
	}
	LogAdapterStats(chosenAdapter);

	if (!CreateDeviceForHighestFeatureLevel(
		featureLevels,
		PUG_COUNT_OF(featureLevels),
		chosenAdapter,
		g_device,
		g_debug_device))
	{
		Error("Failed to create device for the chosen adapter with the provided feature levels");
		return false;
	}

	if (!CreateCommandQueue(
		g_device,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
		D3D12_COMMAND_QUEUE_FLAG_NONE,
		g_commandQueue,
		g_debug_commandQueue))
	{
		Error("Failed to create command queue");
		return false;
	}
	DX12_SET_NAME(g_commandQueue);

	if (!CreateSwapchain(
		factory,
		chosenAdapter,
		g_commandQueue,					//we pass the command queue that will render our final image
		windowsWindow->GetWindowHandle(),
		BACK_BUFFER_FORMAT,
		windowsWindow->GetSize(),
		g_backBufferCount,
		a_fullscreen,
		a_verticalSyncInterval,
		g_swapChain))
	{
		Error("Failed to create swapchain");
		return false;
	}

	SafeRelease(factory);
	SafeRelease(chosenAdapter);

	Info("Initializing Shader Librarian...");
	if (!InitShaderLibrarian())
	{
		PUG_TRY(DestroyGraphics());
		return PUG_RESULT_GRAPHICS_ERROR;
	}
	Message("Success!");

	Info("Creating Descriptor heaps...");
	if (!CreatePersistentDescriptorHeap(
		g_device))
	{
		PUG_TRY(DestroyGraphics());
		return PUG_RESULT_FAILED_TO_CREATE_GPU_RESOURCE;
	}
	Message("Success!");

	//PUG_TRY(CreateConstantBufferPageHeap(g_device, KB(64), D3D12_HEAP_FLAG_DENY_NON_RT_DS_TEXTURES | D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES));
	g_constantBufferHeap = new ConstantBufferHeap(g_device);
	g_guiConstantBufferHeap = new ConstantBufferHeap(g_device);

	Info("Getting back buffers...");
	if (!GetBackbuffers(
		g_device))
	{
		PUG_TRY(DestroyGraphics());
		return PUG_RESULT_FAILED_TO_CREATE_GPU_RESOURCE;
	}
	Message("Success!");

	Info("Creating depth buffers...");
	if (!CreateDepthBuffers(
		g_windowSize.x,
		g_windowSize.y,
		DEPTH_BUFFER_FORMAT))
	{
		PUG_TRY(DestroyGraphics());
		return PUG_RESULT_FAILED_TO_CREATE_GPU_RESOURCE;
	}
	Message("Success!");

	Info("Creating command allocators...");
	if (!CreateCommandAllocators(
		g_device))
	{
		PUG_TRY(DestroyGraphics());
		return PUG_RESULT_GRAPHICS_ERROR;
	}
	Message("Succes!");

	Info("Creating command lists...");
	if (!CreateCommandList(
		g_device))
	{
		PUG_TRY(DestroyGraphics());
		return PUG_RESULT_GRAPHICS_ERROR;
	}
	Message("Success!");

	Info("Creating Root Signatures...");
	if (!CreateRootSignatures(
		g_device))
	{
		PUG_TRY(DestroyGraphics());
		return PUG_RESULT_GRAPHICS_ERROR;
	}
	Message("Success!");

	PUG_TRY(CreatePipelineStateObjects(g_device));

	g_device->CreateFence(1, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_fence));
	DX12_SET_NAME(g_fence);

	g_fenceValue = 1;
	g_fenceEvent = CreateEvent(nullptr, 0, FALSE, nullptr);

	//Info("Initializing syncing objects...");
	//for (uint32_t i = 0; i < g_backBufferCount; ++i)
	//{
	//	if (!InitSyncObject(
	//		g_device,
	//		g_syncObject[i]))
	//	{
	//		PUG_TRY(DX12DestroyGraphics());
	//		return PUG_RESULT_FAILED_TO_CREATE_RENDER_OBJECT;
	//	}
	//}
	//Message("Success!");

	//Info("Creating gui Constant Buffer...");
	//if (!CreateDX12ConstantBuffer(
	//	g_device,
	//	nullptr,
	//	DX12_CONSTANT_BUFFER_ELEMENT_SIZE(sizeof(Matrix4)),
	//	1,
	//	&g_guiConstantBuffer))
	//{
	//	Error("Failed to create gui Constant Buffer");
	//	PUG_TRY(DX12DestroyGraphics());
	//	return PUG_RESULT_FAILED_TO_CREATE_GPU_RESOURCE;
	//}
	//SET_NAME(g_guiConstantBuffer.buffer);
	//Message("Success!");

	return PUG_RESULT_OK;
}

PUG_RESULT pug::assets::graphics::DestroyGraphics()
{
	for (uint32_t i = 0; i < MAX_TEXTURES; ++i)
	{
		DX12DestroyTexture2D(g_textures[i]);	
	}
	for (uint32_t i = 0; i < MAX_VERTEX_BUFFERS; ++i)
	{
		DX12DestroyVertexBuffer(g_vertexBuffers[i]);
	}
	for (uint32_t i = 0; i < MAX_INDEX_BUFFERS; ++i)
	{
		DX12DestroyIndexBuffer(g_indexBuffers[i]);
	}

	SafeRelease(g_rootSignature);
	SafeRelease(g_guiRootSignature);

	SafeRelease(g_pso);
	SafeRelease(g_guiPSO);
	
	SafeRelease(g_commandList);
	SafeRelease(g_resourceCommandList);
	SafeRelease(g_guiCommandList);
#ifdef _DEBUG
	SafeRelease(g_debug_commandList);
	SafeRelease(g_debug_resourceCommandList);
	SafeRelease(g_debug_guiCommandList);
#endif

	for (uint32_t i = 0; i < g_backBufferCount; ++i)
	{
		SafeRelease(g_commandAllocator[i]);
		//g_depthBuffers[i].~DX12Texture2D();
		DX12DestroyTexture2D(g_depthBuffers[i]);
		//g_backBuffers[i].~DX12Texture2D();
		DX12DestroyTexture2D(g_backBuffers[i]);
	}

	delete g_guiConstantBufferHeap;
	g_guiConstantBufferHeap = {};
	delete g_constantBufferHeap;
	g_constantBufferHeap = {};

	//static D3D12_CPU_DESCRIPTOR_HANDLE g_nullCPUSrvHandle;
	//static D3D12_GPU_DESCRIPTOR_HANDLE g_nullGPUSrvHandle;
	//static D3D12_CPU_DESCRIPTOR_HANDLE g_nullCPUDsvHandle;
	//static D3D12_GPU_DESCRIPTOR_HANDLE g_nullGPUDsvHandle;
	//static D3D12_CPU_DESCRIPTOR_HANDLE g_nullCPURtvHandle;
	//static D3D12_GPU_DESCRIPTOR_HANDLE g_nullGPURtvHandle;

	PUG_TRY(DestroyPersistentDescriptorHeap());
	PUG_TRY(DestroyShaderLibrarian());

	SafeRelease(g_swapChain);

	SafeRelease(g_commandQueue);

#ifdef _DEBUG
	SafeRelease(g_debug_commandQueue);
	SafeRelease(g_debug_device);
#endif
	SafeRelease(g_device);

#ifdef _DEBUG
	SafeRelease(g_debugInterface);
#endif
	
	g_viewPort = {};
	g_scissorRect = {};

	g_windowSize = {};
	g_newWindowSize = {};
	g_resizeScreen = 0;

	g_currentBackBufferIndex = 0;

	//destroy sync objects as the very last
	PUG_ASSERT(CloseHandle(g_fenceEvent), "Failed to close fence event handle!");
	g_fenceValue = 0;
	SafeRelease(g_fence);

	return PUG_RESULT_OK;
}

PUG_RESULT pug::assets::graphics::Render(
	const vmath::Matrix4& viewMatrix,
	const vmath::Matrix4& projectionMatrix,
	const vmath::Vector3& cameraPosition,
	const Transform* const a_transforms,
	const Mesh* const a_meshes,
	const uint32_t a_numMeshes,
	const graphics::Mesh* const a_guiMeshes,
	const graphics::Rect* const a_guiScissorRects,
	const Texture2DHandle* const a_guiTextures,
	const uint32_t a_guiMeshCount)
{
	HRESULT result;

	result = g_commandAllocator[g_currentBackBufferIndex]->Reset();
	if (FAILED(result))
	{
		Error("Failed to reset command allocator!");
		return PUG_RESULT_GRAPHICS_ERROR;
	}
	
	size_t meshCBufferBaseOffset = 0;
	size_t guiCBufferBaseOffset = 0;
	PUG_TRY(PrepareFrameData(
		viewMatrix,
		projectionMatrix,
		cameraPosition,
		a_transforms,
		a_meshes,
		a_numMeshes,
		meshCBufferBaseOffset,
		guiCBufferBaseOffset));
	
	if (!RecordCommandList(a_meshes, a_numMeshes, meshCBufferBaseOffset))
	{
		Error("Failed to record command list");
		return PUG_RESULT_FAILED_TO_RENDER;
	}

	if (!RecordGUICommandList(
		a_guiMeshes,
		a_guiScissorRects,
		a_guiTextures,
		a_guiMeshCount,
		guiCBufferBaseOffset))
	{
		Error("Failed to record gui command list");
		return PUG_RESULT_FAILED_TO_RENDER;
	}

	//execute command lists
	ID3D12CommandList* commandLists[] = { g_commandList, g_guiCommandList };
	g_commandQueue->ExecuteCommandLists(PUG_COUNT_OF(commandLists), commandLists);

	if (FAILED(g_swapChain->Present(0, 0)))
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
	
	uint64_t completedValue = g_fence->GetCompletedValue();
	if (localFenceValue > completedValue)
	{
		if (FAILED(g_fence->SetEventOnCompletion(localFenceValue, g_fenceEvent)))
		{
			Error("Failed to set event on completion!");
			return PUG_RESULT_GRAPHICS_ERROR;
		}
		WaitForSingleObject(g_fenceEvent, INFINITE);
	}

	g_currentBackBufferIndex = g_swapChain->GetCurrentBackBufferIndex();

	if (g_resizeScreen)
	{
		if (!RecreateWindowObjects(g_newWindowSize))
		{
			Error("Failed to recreate window objects for resizing the window");
		}
		g_resizeScreen = 0;
	}

	//clear upload buffer
	//memset(g_mappedBackBufferUploadHeapPtr, 0, GetSizeFromFormat(BACK_BUFFER_FORMAT) * g_windowSize.x * g_windowSize.y);

	return PUG_RESULT_OK;
}

//PUG_RESULT pug::assets::graphics::ResizeScreen(
//	const vmath::Int2& windowSize)
//{
//	//not needed, we resize at the end of the frame
//	//WaitForFrameToComplete();
//
//	if (windowSize != g_windowSize)
//	{//no resiziing needed
//		g_newWindowSize = windowSize;
//		g_resizeScreen = 1;
//	}
//
//	return PUG_RESULT_OK;
//}

PUG_RESULT pug::assets::graphics::CreateVertexBuffer(
	const void* a_data,
	const uint64_t a_vertexStride,
	const uint32_t a_vertexCount,
	VertexBufferHandle& out_result,
	const std::string& a_name /* = "" */)
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

	new (&g_vertexBuffers[index]) DX12VertexBuffer(
		resource,
		a_vertexStride,
		a_vertexCount,
		D3D12_RESOURCE_STATE_GENERIC_READ);

	if(a_name.length() > 0)
	{
		std::wstring wname = std::wstring(a_name.begin(), a_name.end());
		resource->SetName(wname.c_str());
	}
	out_result = index;
	return PUG_RESULT_OK;
}

PUG_RESULT pug::assets::graphics::CreateIndexBuffer(
	const void* a_data,
	const PUG_FORMAT a_indexFormat,
	const uint32_t a_indexCount,
	IndexBufferHandle& out_result,
	const std::string& a_name /* = "" */)
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

	new (&g_indexBuffers[index]) DX12IndexBuffer(
		resource,
		indexFormat,
		a_indexCount,
		D3D12_RESOURCE_STATE_GENERIC_READ);

	if (a_name.length() > 0)
	{
		std::wstring wname = std::wstring(a_name.begin(), a_name.end());
		resource->SetName(wname.c_str());
	}
	out_result = index;
	return PUG_RESULT_OK;
}

PUG_RESULT pug::assets::graphics::CreateTexture2D(
	void* a_data,
	uint32_t a_width,
	uint32_t a_height,
	PUG_FORMAT a_format,
	uint32_t a_mipCount,
	Texture2DHandle& out_texture,
	const std::string& a_name /* = "" */)
{
	HRESULT result;
	uint32_t index = PUG_INVALID_ID;

	DXGI_FORMAT format = ConvertFormat(a_format);
	PUG_ASSERT(format != DXGI_FORMAT_UNKNOWN, "Unknown format specified!");

	for (uint32_t i = 0; i < MAX_TEXTURES; ++i)
	{
		if (i != PUG_INVALID_ID && (g_textures[i].IsInitialized() == 0))
		{
			index = i;
			break;
		}
	}
	if (index == PUG_INVALID_ID)
	{//no room in texture array
		return PUG_RESULT_ALLOCATION_FAILURE;
	}

	ID3D12Resource* resource = nullptr;
	//create a commited resource
	HRESULT hr;
	D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Tex2D(format, a_width, a_height, 1, a_mipCount);
	hr = g_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&resource));
	if (FAILED(hr))
	{
		Error("Failed to create commited resource for texture!");
		return PUG_RESULT_FAILED_TO_CREATE_GPU_RESOURCE;
	}

	//Get the size we will need for our uploadHeap
	uint64_t uploadBufferSize = 0;
	g_device->GetCopyableFootprints(&resDesc, 0, a_mipCount, 0, nullptr, nullptr, nullptr, &uploadBufferSize);
	
	//we need an intermediate uploadHeap
	ID3D12Resource* uploadHeap = nullptr;
	D3D12_RESOURCE_DESC uploadResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize); //CreateSimpleBufferResourceDesc(heapSize);
	result = g_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&uploadResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&uploadHeap));
	if (FAILED(result))
	{
		Error("Failed to create upload heap.");
		return false;
	}

	g_resourceCommandList->Reset(g_commandAllocator[g_currentBackBufferIndex], nullptr);

	///<TODO>
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = a_mipCount;

	D3D12_CPU_DESCRIPTOR_HANDLE cpuSRVDescriptor = {};
	D3D12_GPU_DESCRIPTOR_HANDLE gpuSRVDescriptor = {};
	uint32_t srvDescriptorIndex = AllocatePersistentSRVDescriptors();
	if (srvDescriptorIndex == PUG_INVALID_ID)
	{
		Error("Failed to allocated srv descriptors!");
		resource->Release();
		uploadHeap->Release();

		return PUG_RESULT_ALLOCATION_FAILURE;
	}
	PUG_TRY(GetPersistentSRVDescriptors(srvDescriptorIndex, &cpuSRVDescriptor, &gpuSRVDescriptor));
	g_device->CreateShaderResourceView(resource, &srvDesc, cpuSRVDescriptor);

	DX12Texture2D& texture = g_textures[index];

	new (&texture) DX12Texture2D(
					resource,
					srvDescriptorIndex,
					PUG_INVALID_ID,
					PUG_INVALID_ID,
					PUG_INVALID_ID,
					PUG_INVALID_ID,
					a_width,
					a_height,
					format,
					D3D12_RESOURCE_STATE_COPY_DEST);

	size_t bpe = FormatToSize(format);
	D3D12_SUBRESOURCE_DATA data = {};
	data.pData = a_data;
	data.RowPitch = a_width * bpe;
	data.SlicePitch = a_width * a_height * bpe;

	PUG_ASSERT(uploadBufferSize == data.SlicePitch, "Should this be equal, not sure when using compression methods!");

	//Copy the data to the upload heap, and add a command to the command list
	// that we want to copy it from the upload heap to the resource
	UpdateSubresources(g_resourceCommandList, resource, uploadHeap, 0, 0, a_mipCount, &data);

	//Add a barrier to the command list, that changes the state to a
	// D3D12_RESOURCE_STATE_INDEX_BUFFER buffer before using it
	//g_resourceCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(texture->resource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

	g_resourceCommandList->ResourceBarrier(1, &texture.Transition(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

	// Now we execute the command list to upload the initial assets (triangle data)
	g_resourceCommandList->Close();
	ID3D12CommandList* ppCommandLists[] = { g_resourceCommandList };
	g_commandQueue->ExecuteCommandLists(PUG_COUNT_OF(ppCommandLists), ppCommandLists);

	const uint64_t localFenceValue = g_fenceValue;
	g_fenceValue += 1;

	if (FAILED(g_commandQueue->Signal(g_fence, localFenceValue)))
	{
		Warning("Failed to schedule signal fence!");
	}

	uint64_t completedValue = g_fence->GetCompletedValue();
	g_fence->SetEventOnCompletion(localFenceValue, g_fenceEvent);
	if (localFenceValue > completedValue)
	{
		WaitForSingleObject(g_fenceEvent, INFINITE);
	}

	if (a_name.length() > 0)
	{
		std::wstring wname = std::wstring(a_name.begin(), a_name.end());
		resource->SetName(wname.c_str());
	}
	out_texture = index;
	return PUG_RESULT_OK;
}

PUG_RESULT pug::assets::graphics::UpdateVertexBuffer(
	const void* a_data,
	const size_t a_vertexStride,
	const size_t a_vertexCount,
	VertexBufferHandle& inout_vbh)
{
	if (inout_vbh == PUG_INVALID_ID)
	{
		Error("Tried to update an uninitialized vertex buffer");
		return PUG_RESULT_INVALID_ARGUMENTS;
	}
	DX12VertexBuffer& vertexBuffer = g_vertexBuffers[inout_vbh];

	uint64_t currentBufferSize = (uint64_t)vertexBuffer.GetView().SizeInBytes;
	uint64_t desiredBufferSize = a_vertexStride * (uint64_t)a_vertexCount;

	if (desiredBufferSize > currentBufferSize)
	{
		DestroyVertexBuffer(inout_vbh);
		PUG_TRY(CreateVertexBuffer(a_data, a_vertexStride, (uint32_t)a_vertexCount, inout_vbh));
	}
	else
	{//we have enough room 
	 //copy the data
		UINT8* dest = nullptr;
		CD3DX12_RANGE readRange(0, 0);// We do not intend to read from this resource on the CPU.
		ID3D12Resource* resource = vertexBuffer.GetResource();
		resource->Map(0, &readRange, (void**)&dest);
		memcpy(dest, a_data, desiredBufferSize);
		resource->Unmap(0, nullptr);
		//update the vertex buffer
		D3D12_RESOURCE_STATES currState = vertexBuffer.GetCurrentState();
		new (&vertexBuffer) DX12VertexBuffer(
			resource, 
			a_vertexStride, 
			(uint32_t)a_vertexCount, 
			currState);

		//vertexBuffer.view.BufferLocation = vertexBuffer.resource->GetGPUVirtualAddress();//this should not have changed
		//vertexBuffer.view.StrideInBytes = (UINT)vertexStride;
		//vertexBuffer.view.SizeInBytes = (UINT)desiredBufferSize;
	}
	return PUG_RESULT_OK;
}

PUG_RESULT pug::assets::graphics::UpdateIndexBuffer(
	const void* a_data,
	const PUG_FORMAT a_indexFormat,
	const uint32_t a_indexCount,
	IndexBufferHandle& inout_vbh)
{
	if (inout_vbh == PUG_INVALID_ID)
	{
		Error("Tried to update an uninitialized index buffer");
		return PUG_RESULT_INVALID_ARGUMENTS;
	}

	DX12IndexBuffer& indexBuffer = g_indexBuffers[inout_vbh];

	DXGI_FORMAT format = ConvertFormat(a_indexFormat);
	uint64_t currentBufferSize = (uint64_t)indexBuffer.GetView().SizeInBytes;
	uint64_t desiredBufferSize = FormatToSize(format) * (uint64_t)a_indexCount;

	if (desiredBufferSize > currentBufferSize)
	{
		DestroyIndexBuffer(inout_vbh);
		PUG_TRY(CreateIndexBuffer(a_data, a_indexFormat, a_indexCount, inout_vbh));
	}
	else
	{
		{//we have enough room 
		 //copy the data
			UINT8* dest = nullptr;
			CD3DX12_RANGE readRange(0, 0);// We do not intend to read from this resource on the CPU.
			ID3D12Resource* resource = indexBuffer.GetResource();
			resource->Map(0, &readRange, (void**)&dest);
			memcpy(dest, a_data, desiredBufferSize);
			resource->Unmap(0, nullptr);
			//update the index buffer
			D3D12_RESOURCE_STATES currState = indexBuffer.GetCurrentState();
			new (&indexBuffer) DX12IndexBuffer(
				resource, 
				format, 
				a_indexCount, 
				currState);

		}
	}
	return PUG_RESULT_OK;
}

void pug::assets::graphics::DestroyVertexBuffer(
	VertexBufferHandle& vbh)
{
	if(vbh != PUG_INVALID_ID)
	{
		DX12VertexBuffer& vb = g_vertexBuffers[vbh];
		DX12DestroyVertexBuffer(vb);
	}
	vbh = PUG_INVALID_ID;
}

void pug::assets::graphics::DestroyIndexBuffer(
	IndexBufferHandle& ibh)
{
	if (ibh != PUG_INVALID_ID)
	{
		DX12IndexBuffer& ib = g_indexBuffers[ibh];
		DX12DestroyIndexBuffer(ib);
	}
	ibh = PUG_INVALID_ID;
}

void pug::assets::graphics::DestroyTexture2D(
	Texture2DHandle& th)
{
	if (th != PUG_INVALID_ID)
	{
		DX12Texture2D& texture = g_textures[th];
		DX12DestroyTexture2D(texture);
	}
	th = PUG_INVALID_ID;
}

uint32_t pug::assets::graphics::IsVertexBufferValid(VertexBufferHandle& vbh)
{
	return g_vertexBuffers[vbh].IsInitialized();
}
uint32_t pug::assets::graphics::IsIndexBufferValid(IndexBufferHandle& ibh)
{
	return g_indexBuffers[ibh].IsInitialized();
}
uint32_t pug::assets::graphics::IsTextureValid(Texture2DHandle& th)
{
	return g_textures[th].IsInitialized();
}

/*
PUG_RESULT vpl::graphics::DX12CreateTextureFromDDS(
uint8_t* textureData,
uint64_t dataSize,
const vpl::resource::DDS_HEADER& header,
vpl::graphics::TextureID& out_texture)
{
uint32_t index = PUG_INVALID_ID;
for (uint32_t i = 0; i < MAX_TEXTURES; ++i)
{
if (i != PUG_INVALID_ID && (g_textures[i].isInitialized == 0))
{
index = i;
break;
}
}
if (index == PUG_INVALID_ID)
{//no room in texture array
return PUG_RESULT_ARRAY_FULL;
}

UINT width = header.width;
UINT height = header.height;
UINT depth = header.depth;

D3D12_RESOURCE_DIMENSION resDim = D3D12_RESOURCE_DIMENSION_UNKNOWN;
UINT arraySize = 1;
bool isCubeMap = false;
uint32_t mipCount = header.mipMapCount;

if (!(mipCount > 0))
{
Error("Invalid mip count, we must have more than 0 mips");
return PUG_RESULT_INVALID_ARGUMENTS;
}
if (!IsDataFormatBC3Unorm(header.ddspf))
{
Error("File is not of BC3_UNORM format!");
return PUG_RESULT_INVALID_ARGUMENTS;
}
if (mipCount > D3D12_REQ_MIP_LEVELS)
{// Bound sizes (for security purposes we don't trust DDS file metadata larger than the D3D 11.x hardware requirements)
Error("Invalid mip count, no more than 15 mip levels are allowed");
return PUG_RESULT_INVALID_ARGUMENTS;
}

if (header.flags & DDS_HEADER_FLAGS_VOLUME)
{
resDim = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
}
else
{
if (header.caps2 & DDS_CUBEMAP)
{
// We require all six faces to be defined
if ((header.caps2 & DDS_CUBEMAP_ALLFACES) != DDS_CUBEMAP_ALLFACES)
{
Error("Texture file is a cube map but not all faces have been defined.");
return PUG_RESULT_INVALID_ARGUMENTS;
}
arraySize = 6;
isCubeMap = true;
}

depth = 1;
resDim = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
}

switch (resDim)
{
case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
if ((arraySize > D3D12_REQ_TEXTURE1D_ARRAY_AXIS_DIMENSION) ||
(width > D3D12_REQ_TEXTURE1D_U_DIMENSION))
{
Error("Invalide dds texture dimensions");
return PUG_RESULT_INVALID_ARGUMENTS;
}
break;

case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
if (isCubeMap)
{
// This is the right bound because we set arraySize to (NumCubes*6) above
if ((arraySize > D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION) ||
(width > D3D12_REQ_TEXTURECUBE_DIMENSION) ||
(height > D3D12_REQ_TEXTURECUBE_DIMENSION))
{
Error("Invalide dds texture dimensions");
return PUG_RESULT_INVALID_ARGUMENTS;
}
}
else if ((arraySize > D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION) ||
(width > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION) ||
(height > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION))
{
Error("Invalide dds texture dimensions");
return PUG_RESULT_INVALID_ARGUMENTS;
}
break;

case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
if ((arraySize > 1) ||
(width > D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION) ||
(height > D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION) ||
(depth > D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION))
{
Error("Invalide dds texture dimensions");
return PUG_RESULT_INVALID_ARGUMENTS;
}
break;
default:
return PUG_RESULT_INVALID_ARGUMENTS;
}

D3D12_SUBRESOURCE_DATA* d3d12data = (D3D12_SUBRESOURCE_DATA*)_malloca(sizeof(D3D12_SUBRESOURCE_DATA) * (mipCount * arraySize));

int32_t res = FillInitData(width, height, depth, mipCount, arraySize, 0, dataSize, textureData, d3d12data);

if (resDim != D3D12_RESOURCE_DIMENSION_TEXTURE2D)
{
Error("Implement support for other texture types!");
return PUG_RESULT_INVALID_ARGUMENTS;
}

if (res == 0)
{
//get a pointer to our texture
DX12Texture2D* texture = &g_textures[index];
//create a commited resource
HRESULT hr;
D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_BC3_UNORM, width, height, 1, mipCount);
hr = g_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
D3D12_HEAP_FLAG_NONE,
&resDesc,
D3D12_RESOURCE_STATE_COPY_DEST,
nullptr,
IID_PPV_ARGS(&texture->resource));
if (FAILED(hr))
{
Error("Failed to create commited resource for texture!");
return PUG_RESULT_FAILED_TO_CREATE_GPU_RESOURCE;
}

//Get the size we will need for our uploadHeap
uint64_t uploadBufferSize = 0;
g_device->GetCopyableFootprints(&resDesc, 0, mipCount, 0, nullptr, nullptr, nullptr, &uploadBufferSize);
//we need an intermediate uploadHeap
ID3D12Resource* uploadHeap;
if (!CreateTempUploadHeap(uploadBufferSize, uploadHeap))
{
Error("Failed to create texture upload heap.");
return PUG_RESULT_FAILED_TO_CREATE_GPU_RESOURCE;
}

g_resourceCommandList->Reset(g_commandAllocator[g_frameIndex], g_pso);

//Copy the data to the upload heap, and add a command to the command list
// that we want to copy it from the upload heap to the resource
UpdateSubresources(g_resourceCommandList, texture->resource, uploadHeap, 0, 0, mipCount, d3d12data);

//Add a barrier to the command list, that changes the state to a
// D3D12_RESOURCE_STATE_INDEX_BUFFER buffer before using it
g_resourceCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(texture->resource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

// Now we execute the command list to upload the initial assets (triangle data)
g_resourceCommandList->Close();
ID3D12CommandList* ppCommandLists[] = { g_resourceCommandList };
g_commandQueue->ExecuteCommandLists(PUG_COUNT_OF(ppCommandLists), ppCommandLists);
g_syncObject[g_frameIndex].fenceValue++;
if (FAILED(g_commandQueue->Signal(g_syncObject[g_frameIndex].GPUFence, g_syncObject[g_frameIndex].fenceValue)))
{
Error("Failed to signal sync object");
return PUG_RESULT_FAILED_TO_RENDER;
}
Sync(g_syncObject[g_frameIndex]);

///<TODO>
D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
srvDesc.Format = DXGI_FORMAT_BC3_UNORM;
srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
srvDesc.Texture2D.MipLevels = mipCount;

D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptor = {};
D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptor = {};
if (!AllocateDescriptors(g_srvCbvUavHeap, cpuDescriptor, gpuDescriptor))
{
log::Error("Failed to allocate descriptor for texture!");
return PUG_RESULT_ARRAY_FULL;
}
g_device->CreateShaderResourceView(texture->resource, &srvDesc, cpuDescriptor);

texture->cpuSRV = cpuDescriptor;
texture->gpuSRV = gpuDescriptor;
texture->width = width;
texture->height = height;
texture->format = DXGI_FORMAT_BC3_UNORM;
texture->isInitialized = 1;
}
else
{
Error("Failed to set d3d12 data!");
return PUG_RESULT_INVALID_ARGUMENTS;
}

_freea(d3d12data);

out_texture = index;
return PUG_RESULT_OK;
}

PUG_RESULT vpl::graphics::DX12CreateTexture2D(
void* textureData,
uint32_t width,
uint32_t height,
ETextureFormat format,
uint32_t mipCount,
vpl::graphics::TextureID& out_texture)
{
uint32_t index = PUG_INVALID_ID;
for (uint32_t i = 0; i < MAX_TEXTURES; ++i)
{
if (i != PUG_INVALID_ID && (g_textures[i].isInitialized == 0))
{
index = i;
break;
}
}
if (index == PUG_INVALID_ID)
{//no room in texture array
return PUG_RESULT_ARRAY_FULL;
}

//get a pointer to our texture
DX12Texture2D* texture = &g_textures[index];
//create a commited resource
HRESULT hr;
D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Tex2D(ConvertFormatResource(format), width, height, 1, mipCount);
hr = g_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
D3D12_HEAP_FLAG_NONE,
&resDesc,
D3D12_RESOURCE_STATE_COPY_DEST,
nullptr,
IID_PPV_ARGS(&texture->resource));
if (FAILED(hr))
{
Error("Failed to create commited resource for texture!");
return PUG_RESULT_FAILED_TO_CREATE_GPU_RESOURCE;
}

//Get the size we will need for our uploadHeap
uint64_t uploadBufferSize = 0;
g_device->GetCopyableFootprints(&resDesc, 0, mipCount, 0, nullptr, nullptr, nullptr, &uploadBufferSize);
//we need an intermediate uploadHeap
ID3D12Resource* uploadHeap;
if (!CreateTempUploadHeap(uploadBufferSize, uploadHeap))
{
Error("Failed to create texture upload heap.");
return PUG_RESULT_FAILED_TO_CREATE_GPU_RESOURCE;
}

g_resourceCommandList->Reset(g_commandAllocator[g_frameIndex], g_pso);


size_t bpe = GetSizeFromFormat(format);
D3D12_SUBRESOURCE_DATA data = {};
data.pData = textureData;
data.RowPitch = width * bpe;
data.SlicePitch = width * height * bpe;

//Copy the data to the upload heap, and add a command to the command list
// that we want to copy it from the upload heap to the resource
UpdateSubresources(g_resourceCommandList, texture->resource, uploadHeap, 0, 0, mipCount, &data);

//Add a barrier to the command list, that changes the state to a
// D3D12_RESOURCE_STATE_INDEX_BUFFER buffer before using it
g_resourceCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(texture->resource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

// Now we execute the command list to upload the initial assets (triangle data)
g_resourceCommandList->Close();
ID3D12CommandList* ppCommandLists[] = { g_resourceCommandList };
g_commandQueue->ExecuteCommandLists(PUG_COUNT_OF(ppCommandLists), ppCommandLists);
g_syncObject[g_frameIndex].fenceValue++;
if (FAILED(g_commandQueue->Signal(g_syncObject[g_frameIndex].GPUFence, g_syncObject[g_frameIndex].fenceValue)))
{
Error("Failed to signal sync object");
return PUG_RESULT_INTERNAL_GRAPHICS_ERROR;
}
Sync(g_syncObject[g_frameIndex]);

///<TODO>
D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
srvDesc.Format = ConvertFormatResource(format);
srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
srvDesc.Texture2D.MipLevels = mipCount;

D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptor = {};
D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptor = {};
if (!AllocateDescriptors(g_srvCbvUavHeap, cpuDescriptor, gpuDescriptor))
{
log::Error("Failed to allocate descriptor for texture!");
return PUG_RESULT_ARRAY_FULL;
}
g_device->CreateShaderResourceView(texture->resource, &srvDesc, cpuDescriptor);

texture->cpuSRV = cpuDescriptor;
texture->gpuSRV = gpuDescriptor;
texture->width = width;
texture->height = height;
texture->format = ConvertFormatResource(format);
texture->isInitialized = 1;

out_texture = index;
return PUG_RESULT_OK;
}

PUG_RESULT vpl::graphics::DX12UpdateVertexBuffer(
const void* vertices,
const uint64_t vertexStride,
const uint32_t vertexCount,
vpl::graphics::VertexBufferID& inout_result)
{
if (inout_result == PUG_INVALID_ID)
{
Error("Tried to update an uninitialized vertex buffer");
return PUG_RESULT_INVALID_ARGUMENTS;
}
DX12VertexBuffer& vertexBuffer = g_vertexBuffers[inout_result];

uint64_t currentBufferSize = (uint64_t)vertexBuffer.view.SizeInBytes;
uint64_t desiredBufferSize = vertexStride * (uint64_t)vertexCount;

if (desiredBufferSize > currentBufferSize)
{
PUG_TRY(DX12DestroyVertexBuffer(inout_result));
PUG_TRY(DX12CreateVertexBuffer(vertices, vertexStride, vertexCount, inout_result));
}
else
{//we have enough room, copy the data
UINT8* dest = nullptr;
CD3DX12_RANGE readRange(0, 0);// We do not intend to read from this resource on the CPU.
vertexBuffer.resource->Map(0, &readRange, (void**)&dest);
memcpy(dest, vertices, desiredBufferSize);
vertexBuffer.resource->Unmap(0, nullptr);
//update the view
vertexBuffer.view.BufferLocation = vertexBuffer.resource->GetGPUVirtualAddress();//this should not have changed
vertexBuffer.view.StrideInBytes = (UINT)vertexStride;
vertexBuffer.view.SizeInBytes = (UINT)desiredBufferSize;
}
return PUG_RESULT_OK;
}

PUG_RESULT vpl::graphics::DX12UpdateIndexBuffer(
const void* indices,
const uint64_t indexStride,
const uint32_t indexCount,
vpl::graphics::IndexBufferID& inout_result)
{
if (inout_result == PUG_INVALID_ID)
{
Error("Tried to update an uninitialized index buffer");
return PUG_RESULT_INVALID_ARGUMENTS;
}

DX12IndexBuffer& indexBuffer = g_indexBuffers[inout_result];

uint64_t currentBufferSize = (uint64_t)indexBuffer.view.SizeInBytes;
uint64_t desiredBufferSize = indexStride * (uint64_t)indexCount;

if (desiredBufferSize > currentBufferSize)
{
PUG_TRY(DX12DestroyIndexBuffer(inout_result));
PUG_TRY(DX12CreateIndexBuffer(indices, indexStride, indexCount, inout_result));
}
else
{
//we have enough room, copy the data
UINT8* dest = nullptr;
CD3DX12_RANGE readRange(0, 0);// We do not intend to read from this resource on the CPU.
indexBuffer.resource->Map(0, &readRange, (void**)&dest);
memcpy(dest, indices, desiredBufferSize);
indexBuffer.resource->Unmap(0, nullptr);
//update the view
indexBuffer.view.BufferLocation = indexBuffer.resource->GetGPUVirtualAddress();//this should not have changed
indexBuffer.view.Format = GetIndexFormatFromSize(indexStride);
indexBuffer.view.SizeInBytes = (UINT)desiredBufferSize;

}
return PUG_RESULT_OK;
}

PUG_RESULT vpl::graphics::DX12DestroyVertexBuffer(
vpl::graphics::VertexBufferID& vertexBuffer)
{
DX12VertexBuffer* vb = &g_vertexBuffers[vertexBuffer];
if (vb->isInitialized)
{
//release vertex buffer
if (!DX12DestroyVertexBuffer(
vb))
{
Error("Failed to destroy vertex buffer");
return PUG_RESULT_FAILED_TO_DELETE_GPU_RESOURCE;
}
}
else
{
Warning("Tried to destroy uninitialized vertex buffer!");
return PUG_RESULT_INVALID_ARGUMENTS;
}

//no aabb was present for this vertex buffer
//else
//{
//	Warning("Tried to destroy uninitialized mesh aabb vertex buffer!");
//	return RESULT_INVALID_ARGUMENTS;
//}

vertexBuffer = 0;
return PUG_RESULT_OK;
}

PUG_RESULT vpl::graphics::DX12DestroyIndexBuffer(
vpl::graphics::IndexBufferID& indexBuffer)
{
DX12IndexBuffer* ib = &g_indexBuffers[indexBuffer];
if (ib->isInitialized)
{
//release index buffer
if (!DX12DestroyIndexBuffer(
ib))
{
Error("Failed to destroy index buffer");
return PUG_RESULT_FAILED_TO_DELETE_GPU_RESOURCE;
}
indexBuffer = 0;
return PUG_RESULT_OK;
}
else
{
Warning("Triend to destroy uninitialized index buffer!");
return PUG_RESULT_INVALID_ARGUMENTS;
}
}

PUG_RESULT vpl::graphics::DX12DestroyTexture(
vpl::graphics::TextureID& texture)
{
WaitForFrameToComplete();
return PUG_RESULT_UNKNOWN;
}

bool vpl::graphics::DX12IsVertexBufferValid(
vpl::graphics::VertexBufferID& vertexBuffer)
{
return false;
}
bool vpl::graphics::DX12IsIndexBufferValid(
vpl::graphics::IndexBufferID& indexBuffer)
{
return false;
}
bool vpl::graphics::DX12IsTextureValid(
vpl::graphics::TextureID& texture)
{
return false;
}
*/