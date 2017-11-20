#include "dx12_vertex_buffer.h"

using namespace pug;
using namespace pug::assets;
using namespace pug::assets::graphics;

DX12VertexBuffer::DX12VertexBuffer(
	ID3D12Resource* a_resource,
	const size_t a_vertexStride_bytes,
	const uint32_t a_vertexCount,
	const D3D12_RESOURCE_STATES a_initialState)
{
	m_view =
	{
		a_resource->GetGPUVirtualAddress(),
		(uint32_t)(a_vertexStride_bytes * a_vertexCount),
		(uint32_t)a_vertexStride_bytes,
	};

	m_vertexCount = a_vertexCount;
	m_resource = a_resource;
	m_currentState = a_initialState;
	m_initialized = 1;
}

DX12VertexBuffer::~DX12VertexBuffer()
{
	m_view = {};
	m_vertexCount = {};
}