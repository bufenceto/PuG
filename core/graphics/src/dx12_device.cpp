#include "dx12_device.h"
#include "logger.h"
#include <dxgi1_4.h>
#include "d3dx12.h"

namespace pug
{
namespace graphics
{
	DX12Device::DX12Device(ID3D12Device1* a_device)
		: m_device(a_device)
	{

	}

	DX12Device::~DX12Device()
	{
		m_device->Release();
	}

	PUG_RESULT DX12Device::CreateCommandQueue(ID3D12CommandQueue *& out_commandQueue, D3D12_COMMAND_LIST_TYPE a_type, D3D12_COMMAND_QUEUE_PRIORITY a_priority, D3D12_COMMAND_QUEUE_FLAGS a_flags)
	{
		D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
		commandQueueDesc.Flags = a_flags;
		commandQueueDesc.Priority = a_priority;
		commandQueueDesc.Type = a_type;
		commandQueueDesc.NodeMask = 0;
		if (FAILED(m_device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&out_commandQueue))))
		{
			log::Error("Failed to create command queue.");
			return PUG_RESULT_GRAPHICS_ERROR;
		}

		return PUG_RESULT_OK;
	}

	PUG_RESULT DX12Device::CreateGraphicsCommandList(ID3D12GraphicsCommandList *& out_commandList, ID3D12CommandAllocator * a_commandAllocator, D3D12_COMMAND_LIST_TYPE a_type, ID3D12PipelineState * a_pso)
	{
		if (FAILED(m_device->CreateCommandList(0, a_type, a_commandAllocator, a_pso, IID_PPV_ARGS(&out_commandList))))
		{
			log::Error("Failed to create graphics command list.");
			return PUG_RESULT_GRAPHICS_ERROR;
		}

		return PUG_RESULT_OK;
	}

	PUG_RESULT DX12Device::CreateCommandAllocator(ID3D12CommandAllocator *& out_commandAllocator, D3D12_COMMAND_LIST_TYPE a_type)
	{
		if (FAILED(m_device->CreateCommandAllocator(a_type, IID_PPV_ARGS(&out_commandAllocator))))
		{
			log::Error("Error creating command allocators.");
			return PUG_RESULT_GRAPHICS_ERROR;
		}

		return PUG_RESULT_OK;
	}

	PUG_RESULT DX12Device::CreateDescriptorHeap(ID3D12DescriptorHeap *& out_descriptorHeap, D3D12_DESCRIPTOR_HEAP_TYPE a_type, uint32_t a_numDescriptors, D3D12_DESCRIPTOR_HEAP_FLAGS a_flags)
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.Flags = a_flags;
		desc.Type = a_type;
		desc.NodeMask = 0;
		desc.NumDescriptors = a_numDescriptors;

		if (FAILED(m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&out_descriptorHeap))))
		{
			log::Error("Error creating descriptor heap.");
			return PUG_RESULT_GRAPHICS_ERROR;
		}

		return PUG_RESULT_OK;
	}

	uint32_t DX12Device::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE a_type)
	{
		return m_device->GetDescriptorHandleIncrementSize(a_type);
	}

	PUG_RESULT DX12Device::CreateCommittedResource(ID3D12Resource *& a_outResource, D3D12_HEAP_PROPERTIES * a_pHeapProperties, D3D12_HEAP_FLAGS a_flags, D3D12_RESOURCE_DESC * a_pResourceDesc, D3D12_RESOURCE_STATES a_initialState, D3D12_CLEAR_VALUE * a_clearValue)
	{
		if (FAILED(m_device->CreateCommittedResource(
			a_pHeapProperties,
			a_flags,
			a_pResourceDesc,
			a_initialState,
			a_clearValue,
			IID_PPV_ARGS(&a_outResource))))
		{
			log::Error("Error creating committed resource.");
			return PUG_RESULT_GRAPHICS_ERROR;
		}

		return PUG_RESULT_OK;
	}

	PUG_RESULT DX12Device::CreateVertexAndIndexBuffer(ID3D12Resource *& out_vertexBuffer, ID3D12Resource *& out_indexBuffer, D3D12_VERTEX_BUFFER_VIEW& out_vertexBufferView, D3D12_INDEX_BUFFER_VIEW& out_indexBufferView, Vertex * vertexArray, uint32_t vertexCount, uint32_t * indexArray, uint32_t indexCount)
	{
		// Vertex buffer
		{
			const uint32_t vertexBufferSize = sizeof(Vertex) * vertexCount;

			if (!PUG_SUCCEEDED(CreateCommittedResource(
				out_vertexBuffer,
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr
			)))
			{
				return PUG_RESULT_GRAPHICS_ERROR;
			}

			// Copy the triangle data to the vertex buffer.
			uint8_t* pVertexDataBegin;
			CD3DX12_RANGE readRange(0, 0);
			if (FAILED(out_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin))))
			{
				log::Error("Error mapping vertex buffer.");
				return PUG_RESULT_GRAPHICS_ERROR;
			}
			memcpy(pVertexDataBegin, vertexArray, vertexBufferSize);
			out_vertexBuffer->Unmap(0, nullptr);

			out_vertexBufferView.BufferLocation = out_vertexBuffer->GetGPUVirtualAddress();
			out_vertexBufferView.StrideInBytes = sizeof(Vertex);
			out_vertexBufferView.SizeInBytes = vertexBufferSize;

		}

		//Index buffer
		{
			const uint32_t indexBufferSize = sizeof(uint32_t) * indexCount;

			if (!PUG_SUCCEEDED(CreateCommittedResource(
				out_indexBuffer,
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize),
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr
				)))
			{
				return PUG_RESULT_GRAPHICS_ERROR;
			}

			// Copy the triangle data to the vertex buffer.
			uint8_t* pIndexDataBegin;
			CD3DX12_RANGE readRange(0, 0);
			if (FAILED(out_indexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin))))
			{
				log::Error("Error mapping index buffer.");
				return PUG_RESULT_GRAPHICS_ERROR;
			}
			memcpy(pIndexDataBegin, indexArray, indexBufferSize);
			out_indexBuffer->Unmap(0, nullptr);

			out_indexBufferView.BufferLocation = out_indexBuffer->GetGPUVirtualAddress();
			out_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
			out_indexBufferView.SizeInBytes = indexBufferSize;
		}

		return PUG_RESULT_OK;
	}

	PUG_RESULT DX12Device::CreateGraphicsPipelineState(ID3D12PipelineState *& out_pso, D3D12_GRAPHICS_PIPELINE_STATE_DESC & a_desc)
	{
		if (FAILED(m_device->CreateGraphicsPipelineState(&a_desc, IID_PPV_ARGS(&out_pso))))
		{
			log::Error("Failed to create graphics pipeline state object.");
			return PUG_RESULT_GRAPHICS_ERROR;
		}

		return PUG_RESULT_OK;
	}

	PUG_RESULT DX12Device::CreateVersionedRootSignature(ID3D12RootSignature *& out_rootSignature, D3D12_VERSIONED_ROOT_SIGNATURE_DESC & a_desc, D3D_ROOT_SIGNATURE_VERSION a_maxVersion)
	{
		// Serialize description
		ID3DBlob* rootDescriptionBlob;
		ID3DBlob* errorBlob;

		if (FAILED(D3DX12SerializeVersionedRootSignature(&a_desc, a_maxVersion, &rootDescriptionBlob, &errorBlob)))
		{
			log::Error("Failed to serialize root signature.\nError: %s", (char*)errorBlob->GetBufferPointer());
			if (errorBlob)
				errorBlob->Release();
			if (rootDescriptionBlob)
				rootDescriptionBlob->Release();
			return PUG_RESULT_GRAPHICS_ERROR;
		}

		if (FAILED(m_device->CreateRootSignature(
			0,
			rootDescriptionBlob->GetBufferPointer(),
			rootDescriptionBlob->GetBufferSize(),
			IID_PPV_ARGS(&out_rootSignature)
		)))
		{
			log::Error("Failed to create root signature.");
			if (errorBlob)
				errorBlob->Release();
			if (rootDescriptionBlob)
				rootDescriptionBlob->Release();
			return PUG_RESULT_GRAPHICS_ERROR;
		}

		if (errorBlob)
			errorBlob->Release();
		if (rootDescriptionBlob)
			rootDescriptionBlob->Release();

		return PUG_RESULT_OK;
	}

	void DX12Device::CreateRenderTargetView(ID3D12Resource * a_resource, D3D12_RENDER_TARGET_VIEW_DESC* a_pDesc, D3D12_CPU_DESCRIPTOR_HANDLE a_handle)
	{
		m_device->CreateRenderTargetView(a_resource, a_pDesc, a_handle);
	}

	PUG_RESULT DX12Device::CreateFence(ID3D12Fence *& out_fence, uint64_t a_initialValue, D3D12_FENCE_FLAGS a_flags)
	{
		if (FAILED(m_device->CreateFence(a_initialValue, a_flags, IID_PPV_ARGS(&out_fence))))
		{
			log::Error("Failed to create fence sync object.");
			return PUG_RESULT_GRAPHICS_ERROR;
		}

		return PUG_RESULT_OK;
	}

}
}