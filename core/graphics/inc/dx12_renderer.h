#pragma once
#include <d3d12.h>
#include <dxgi1_4.h>
#include "renderer_interface.h"
#include "dx12_device.h"

namespace pug {
namespace graphics {

	class DX12Renderer : public IRenderer
	{
	public:
		DX12Renderer();
		~DX12Renderer() {}

		const static uint32_t BufferCount = 2;

		virtual PUG_RESULT Initialize(Window* a_window)	override;
		virtual PUG_RESULT Resize(Window* a_window)		override;
		virtual void Draw()							override;
		virtual void Destroy()						override;

	private:

		void PopulateCommandList();
		void TransitionToNextFrame();

		PUG_RESULT LoadPipeline(Window* a_window);
		PUG_RESULT LoadAssets();

		DX12Device* m_device;
		IDXGISwapChain3* m_swapChain;

		// Descriptor heaps

		ID3D12DescriptorHeap* m_rtvDescriptorHeap;
		uint32_t m_rtvDescriptorSize;

		// Render target resources

		ID3D12Resource* m_OMTargets[BufferCount];

		// Command queue/list/allocators

		ID3D12CommandQueue* m_directCommandQueue;
		ID3D12CommandAllocator* m_directCommandAllocators[BufferCount];
		ID3D12GraphicsCommandList* m_directCommandList;

		// Pipeline state resources

		ID3D12RootSignature* m_rootSignature;
		ID3D12PipelineState* m_PSO;

		// Synchronization objects

		ID3D12Fence* m_fence;
		uint32_t m_fenceValues[BufferCount];
		HANDLE m_fenceEvent;

		// Frame data

		uint8_t m_currentFrameIndex;

		D3D12_VIEWPORT m_viewport;
		D3D12_RECT m_scissorRect;
		float m_aspectRatio;

		// TEMPORARY ---- REMOVE!!!!!!!!
		ID3D12Resource* m_vertexBuffer;
		ID3D12Resource* m_indexBuffer;
		D3D12_VERTEX_BUFFER_VIEW m_vbView;
		D3D12_INDEX_BUFFER_VIEW m_ibView;


	};
}
}