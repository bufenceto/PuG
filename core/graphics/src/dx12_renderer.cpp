#include "dx12_renderer.h"
#include "win32_window.h"
#include "d3dx12.h"
#include <d3dcompiler.h>
#include "logger.h"
#include <codecvt>
#include <experimental\filesystem>
#include <comdef.h>
#include "vertex.h"

namespace pug {
namespace graphics {

	std::string GetLastErrorAsString()
	{
		//Get the error message, if any.
		DWORD errorMessageID = ::GetLastError();
		if (errorMessageID == 0)
			return std::string(); //No error message has been recorded

		LPSTR messageBuffer = nullptr;
		size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

		std::string message(messageBuffer, size);

		//Free the buffer.
		LocalFree(messageBuffer);

		return message;
	}

	const char* GetFeatureLevelString(const D3D_FEATURE_LEVEL &featureLevel)
	{
		switch (featureLevel)
		{
		case D3D_FEATURE_LEVEL_11_0:
			return "D3D_FEATURE_LEVEL_11_0";

		case D3D_FEATURE_LEVEL_11_1:
			return "D3D_FEATURE_LEVEL_11_1";

		case D3D_FEATURE_LEVEL_12_0:
			return "D3D_FEATURE_LEVEL_12_0";

		case D3D_FEATURE_LEVEL_12_1:
			return "D3D_FEATURE_LEVEL_12_1";

		default:
			return "INVALID";
		}

	}

	const std::string ws2s(std::wstring ws)
	{
		using convert_type = std::codecvt_utf8<wchar_t>;
		std::wstring_convert<convert_type, wchar_t> converter;
		std::string converted_str = converter.to_bytes(ws);
		return converted_str;
	}

	DX12Renderer::DX12Renderer()
		: m_currentFrameIndex(0)
	{
		for (uint32_t i = 0; i < BufferCount; ++i)
		{
			m_fenceValues[i] = 0;
		}
	}

	PUG_RESULT DX12Renderer::Initialize(Window* a_window)
	{
		vmath::Int2 size = a_window->GetSize();
		m_viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, size.x, size.y, 0.0f, 1.0f);
		m_scissorRect = CD3DX12_RECT(0, 0, static_cast<LONG>(size.x), static_cast<LONG>(size.y));
		m_aspectRatio = (float)size.x / (float)size.y;

		if (!PUG_SUCCEEDED(LoadPipeline(a_window)))
			return PUG_RESULT_GRAPHICS_ERROR;
		if (!PUG_SUCCEEDED(LoadAssets()))
			return PUG_RESULT_GRAPHICS_ERROR;

		return PUG_RESULT_OK;
	}

	PUG_RESULT DX12Renderer::LoadPipeline(Window* a_window)
	{
		// Enable debug layer
#if defined (_DEBUG)
		{
			ID3D12Debug* debugController;
			if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
			{
				debugController->EnableDebugLayer();
				debugController->Release();
			}
		}
#endif
		// Create DXGI factory
		IDXGIFactory4* factory;

		if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory))))
		{
			log::Error("Unable to create DXGI factory.\n");
			return PUG_RESULT_GRAPHICS_ERROR;
		}

		// Query adapters
		IDXGIAdapter1* adapter;
		DXGI_ADAPTER_DESC1 adapterDesc;
		{
			IDXGIAdapter1* current;
			size_t maxVRAM = 0;
			for (uint32_t adapterIndices = 0; DXGI_ERROR_NOT_FOUND != factory->EnumAdapters1(adapterIndices, &current); ++adapterIndices)
			{
				DXGI_ADAPTER_DESC1 currentDesc;
				current->GetDesc1(&currentDesc);

				if (currentDesc.Flags & D3D_DRIVER_TYPE_SOFTWARE)
				{
					continue;
				}

				if (maxVRAM <= currentDesc.DedicatedVideoMemory)
				{
					adapter = current;
					maxVRAM = currentDesc.DedicatedVideoMemory;
					adapterDesc = currentDesc;
				}
			}
		}

		D3D_FEATURE_LEVEL supportedFeatureLevels[] =
		{
			D3D_FEATURE_LEVEL_12_1,
			D3D_FEATURE_LEVEL_12_0,
			D3D_FEATURE_LEVEL_11_1,
			D3D_FEATURE_LEVEL_11_0
		};

		ID3D12Device1* tDevice;
		// Create D3D12 device

		bool found = false;

		for (uint32_t i = 0; i < _countof(supportedFeatureLevels) && !found; ++i)
		{
			if (SUCCEEDED(D3D12CreateDevice(adapter, supportedFeatureLevels[i], __uuidof(ID3D12Device1), nullptr)))
			{
				if (SUCCEEDED(D3D12CreateDevice(adapter, supportedFeatureLevels[i], IID_PPV_ARGS(&tDevice))))
				{
					log::Info("Created device at feature level %s.\nAdapter used: %s", GetFeatureLevelString(supportedFeatureLevels[i]), ws2s(adapterDesc.Description));
					found = true;
					break;
				}
			}
		}

		if (!tDevice)
		{
			log::Error("Failed to create D3D12 device.");
			return PUG_RESULT_GRAPHICS_ERROR;
		}

		m_device = new DX12Device(tDevice);

		// Release adapter
		adapter->Release();

		// Create a graphics (direct) command queue

		if (!PUG_SUCCEEDED(m_device->CreateCommandQueue(m_directCommandQueue)))
		{
			return PUG_RESULT_GRAPHICS_ERROR;
		}

		// Create swap chain
		{
			vmath::Int2 size = a_window->GetSize();

			DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
			swapChainDesc.BufferCount = BufferCount;
			swapChainDesc.Width = size.x;
			swapChainDesc.Height = size.y;
			swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			swapChainDesc.SampleDesc.Count = 1;
			swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
			swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

			HWND tempHandle = dynamic_cast<Win32Window*>(a_window)->GetWindowHandle();

			if (FAILED(factory->CreateSwapChainForHwnd(
				m_directCommandQueue,
				dynamic_cast<Win32Window*>(a_window)->GetWindowHandle(),
				&swapChainDesc,
				nullptr,
				nullptr,
				(IDXGISwapChain1**)&m_swapChain
			)))
			{
				log::Error("Error creating swap chain.");
				return PUG_RESULT_GRAPHICS_ERROR;
			}
		}

		// Create RTV descriptor heap
		if (!PUG_SUCCEEDED(m_device->CreateDescriptorHeap(m_rtvDescriptorHeap, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, BufferCount)))
		{
			return PUG_RESULT_GRAPHICS_ERROR;
		}

		m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);


		// Create render targets and command allocators
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

		for (uint32_t i = 0; i < BufferCount; ++i)
		{
			if (FAILED(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_OMTargets[i]))))
			{
				log::Error("Error getting swap chain buffer[%d].", i);
				return PUG_RESULT_GRAPHICS_ERROR;
			}
			m_device->CreateRenderTargetView(m_OMTargets[i], {}, rtvHandle);
			rtvHandle.Offset(1, m_rtvDescriptorSize);

			if (!PUG_SUCCEEDED(m_device->CreateCommandAllocator(m_directCommandAllocators[i])))
			{
				return PUG_RESULT_GRAPHICS_ERROR;
			}
		}

		// Create root signature

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc;
		desc.Init_1_1(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		if(!PUG_SUCCEEDED(m_device->CreateVersionedRootSignature(m_rootSignature, desc, D3D_ROOT_SIGNATURE_VERSION_1_0)))
		{
			return PUG_RESULT_GRAPHICS_ERROR;
		}

		// Create pipeline state object
		{
			// Compile shaders

			ID3DBlob *vertexShader = nullptr, *pixelShader = nullptr;

			std::experimental::filesystem::path shaderPath = L"graphics/rsc/default.hlsl";
			if (!std::experimental::filesystem::exists(shaderPath))
			{
				printf("Could not find file: %s", shaderPath.string().c_str());
			}

			ID3DBlob* error;

			if (FAILED(D3DCompileFromFile(shaderPath.c_str(), nullptr, nullptr, "VSMain", "vs_5_0", 0, 0, &vertexShader, &error)))
			{
				log::Error("Error compiling vertex shader. Error message: %s", (char*)error->GetBufferPointer());
			}
			if (FAILED(D3DCompileFromFile(shaderPath.c_str(), nullptr, nullptr, "PSMain", "ps_5_0", 0, 0, &pixelShader, &error)))
			{
				log::Error("Error compiling pixel shader. Error message: %s", (char*)error->GetBufferPointer());
			}

			// Create input layout

			D3D12_INPUT_ELEMENT_DESC elementDescs[] =
			{
				{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
				{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
			};

			D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};
			inputLayoutDesc.NumElements = _countof(elementDescs);
			inputLayoutDesc.pInputElementDescs = elementDescs;

			// Create PSO

			D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
			desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
			desc.NumRenderTargets = 1;
			desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			desc.pRootSignature = m_rootSignature;
			desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
			desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
			desc.SampleDesc.Count = 1;
			desc.SampleMask = UINT_MAX;
			desc.VS = CD3DX12_SHADER_BYTECODE(vertexShader);
			desc.PS = CD3DX12_SHADER_BYTECODE(pixelShader);
			desc.InputLayout = inputLayoutDesc;
			
			if (!PUG_SUCCEEDED(m_device->CreateGraphicsPipelineState(m_PSO, desc)))
			{
				return PUG_RESULT_GRAPHICS_ERROR;
			}

			if (error)
				error->Release();

			vertexShader->Release();
			pixelShader->Release();
		}

		// Create command list

		if (!PUG_SUCCEEDED(m_device->CreateGraphicsCommandList(
			m_directCommandList,
			m_directCommandAllocators[m_swapChain->GetCurrentBackBufferIndex()],
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			m_PSO
		)))
		{
			return PUG_RESULT_GRAPHICS_ERROR;
		}

		m_directCommandList->Close();

		// Create synchroniztion objects
		{
			if (!PUG_SUCCEEDED(m_device->CreateFence(m_fence)))
			{
				return PUG_RESULT_GRAPHICS_ERROR;
			}

			m_fenceEvent = CreateEvent(nullptr, false, false, nullptr);
			if (!m_fenceEvent)
			{
				log::Error("Failed to create a fence event.");
				return PUG_RESULT_GRAPHICS_ERROR;
			}
		}

		factory->Release();
		return PUG_RESULT_OK;
	}


	PUG_RESULT DX12Renderer::LoadAssets()
	{
		Vertex triangleVertices[] =
		{
			{ { -0.25f, 0.25f * m_aspectRatio, 0.0f },{ 1.0f, 0.0f, 0.0f, 1.0f } },
			{ { 0.25f, 0.25f * m_aspectRatio, 0.0f },{ 0.0f, 1.0f, 0.0f, 1.0f } },
			{ { 0.25f, -0.25f * m_aspectRatio, 0.0f },{ 0.0f, 0.0f, 1.0f, 1.0f } },
			{ { -0.25f, -0.25f * m_aspectRatio, 0.0f },{ 1.0f, 1.0f, 1.0f, 1.0f } }
		};

		uint32_t indices[] =
		{
			0, 1, 3, 3, 1, 2
		};

		if (!PUG_SUCCEEDED(m_device->CreateVertexAndIndexBuffer(m_vertexBuffer, m_indexBuffer, m_vbView, m_ibView, triangleVertices, _countof(triangleVertices), indices, _countof(indices))))
		{
			return PUG_RESULT_GRAPHICS_ERROR;
		}

		return PUG_RESULT_OK;
	}

	PUG_RESULT DX12Renderer::Resize(Window* window)
	{

		return PUG_RESULT_OK;
	}

	void DX12Renderer::Draw()
	{
		PopulateCommandList();
		TransitionToNextFrame();
	}

	void DX12Renderer::PopulateCommandList()
	{
		m_directCommandAllocators[m_currentFrameIndex]->Reset();
		m_directCommandList->Reset(m_directCommandAllocators[m_currentFrameIndex], m_PSO);

		m_directCommandList->SetGraphicsRootSignature(m_rootSignature);

		CD3DX12_RESOURCE_BARRIER renderTargetResourceBarrier =
			CD3DX12_RESOURCE_BARRIER::Transition(m_OMTargets[m_currentFrameIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

		m_directCommandList->ResourceBarrier(1, &renderTargetResourceBarrier);

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
		rtvHandle.Offset(m_currentFrameIndex, m_rtvDescriptorSize);

		m_directCommandList->OMSetRenderTargets(1, &rtvHandle, false, nullptr);
		m_directCommandList->RSSetViewports(1, &m_viewport);
		m_directCommandList->RSSetScissorRects(1, &m_scissorRect);

		float color[4] = { 0.0f, 0.4f, 0.5f, 0.5f };
		m_directCommandList->ClearRenderTargetView(rtvHandle, color, 0, nullptr);

		m_directCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_directCommandList->IASetVertexBuffers(0, 1, &m_vbView);
		m_directCommandList->IASetIndexBuffer(&m_ibView);
		m_directCommandList->DrawIndexedInstanced(6, 1, 0, 0, 0);

		// Indicate that the render target will now be used to present when the command list is done executing.
		CD3DX12_RESOURCE_BARRIER presentResourceBarrier =
			CD3DX12_RESOURCE_BARRIER::Transition(m_OMTargets[m_currentFrameIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

		m_directCommandList->ResourceBarrier(1, &presentResourceBarrier);

		m_directCommandList->Close();

		ID3D12CommandList* ppCommandLists[] =
		{
			m_directCommandList
		};

		m_directCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
		m_swapChain->Present(1, 0);
	}

	void DX12Renderer::TransitionToNextFrame()
	{
		//Schedule signal
		const UINT currFenceValue = m_fenceValues[m_currentFrameIndex];

		if (FAILED(m_directCommandQueue->Signal(m_fence, currFenceValue)))
		{
			log::Error("Error scheduling signal.");
		}

		//Update frame index
		m_currentFrameIndex = m_swapChain->GetCurrentBackBufferIndex();

		//Wait
		if (m_fence->GetCompletedValue() < m_fenceValues[m_currentFrameIndex])
		{
			m_fence->SetEventOnCompletion(m_fenceValues[m_currentFrameIndex], m_fenceEvent);
			WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);
		}

		m_fenceValues[m_currentFrameIndex] = currFenceValue + 1;
	}

	void DX12Renderer::Destroy()
	{


	}



}
}