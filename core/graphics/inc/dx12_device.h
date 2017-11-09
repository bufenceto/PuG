#pragma once
#include <d3d12.h>
#include <cstdint>
#include "vertex.h"
#include "result_codes.h"

namespace pug
{
namespace graphics
{
	class DX12Device
	{
	public:
		DX12Device(ID3D12Device1* a_device);
		~DX12Device();

		PUG_RESULT CreateCommandQueue(
			ID3D12CommandQueue*& out_commandQueue,
			D3D12_COMMAND_LIST_TYPE a_type = D3D12_COMMAND_LIST_TYPE_DIRECT,
			D3D12_COMMAND_QUEUE_PRIORITY a_priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
			D3D12_COMMAND_QUEUE_FLAGS a_flags = D3D12_COMMAND_QUEUE_FLAG_NONE
		);

		PUG_RESULT CreateGraphicsCommandList(
			ID3D12GraphicsCommandList*& out_commandList,
			ID3D12CommandAllocator* a_commandAllocator,
			D3D12_COMMAND_LIST_TYPE a_type = D3D12_COMMAND_LIST_TYPE_DIRECT,
			ID3D12PipelineState* a_pso = nullptr
		);

		PUG_RESULT CreateCommandAllocator(
			ID3D12CommandAllocator*& out_commandAllocator,
			D3D12_COMMAND_LIST_TYPE a_type = D3D12_COMMAND_LIST_TYPE_DIRECT
		);

		PUG_RESULT CreateDescriptorHeap(
			ID3D12DescriptorHeap*& out_descriptorHeap,
			D3D12_DESCRIPTOR_HEAP_TYPE a_type,
			uint32_t a_numDescriptors,
			D3D12_DESCRIPTOR_HEAP_FLAGS a_flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE
		);

		uint32_t GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE a_type);

		PUG_RESULT CreateCommittedResource(
			ID3D12Resource*& a_outResource,
			D3D12_HEAP_PROPERTIES* a_pHeapProperties,
			D3D12_HEAP_FLAGS a_flags,
			D3D12_RESOURCE_DESC* a_pResourceDesc,
			D3D12_RESOURCE_STATES a_initialState,
			D3D12_CLEAR_VALUE* a_clearValue
		);

		PUG_RESULT CreateVertexAndIndexBuffer(
			ID3D12Resource*& out_vertexBuffer,
			ID3D12Resource*& out_indexBuffer,
			D3D12_VERTEX_BUFFER_VIEW& out_vertexBufferView,
			D3D12_INDEX_BUFFER_VIEW& out_indexBufferView,
			Vertex* vertexArray,
			uint32_t vertexCount,
			uint32_t* indexArray,
			uint32_t indexCount
		);

		PUG_RESULT CreateGraphicsPipelineState(
			ID3D12PipelineState*& out_pso,
			D3D12_GRAPHICS_PIPELINE_STATE_DESC &a_desc
		);

		PUG_RESULT CreateVersionedRootSignature(
			ID3D12RootSignature*& out_rootSignature,
			D3D12_VERSIONED_ROOT_SIGNATURE_DESC &a_desc,
			D3D_ROOT_SIGNATURE_VERSION a_maxVersion = D3D_ROOT_SIGNATURE_VERSION_1_1
		);

		void CreateRenderTargetView(
			ID3D12Resource* a_resource,
			D3D12_RENDER_TARGET_VIEW_DESC* a_pDesc,
			D3D12_CPU_DESCRIPTOR_HANDLE a_handle
		);

		PUG_RESULT CreateFence(
			ID3D12Fence*& out_fence,
			uint64_t a_initialValue = 0,
			D3D12_FENCE_FLAGS a_flags = D3D12_FENCE_FLAG_NONE 
		);

	private:
		ID3D12Device1* m_device;
	};
}
}