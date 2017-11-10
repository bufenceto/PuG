#include "dx12_index_buffer.h"

#include "../dx12_helper.h"

using namespace pug;
using namespace pug::assets;
using namespace pug::assets::graphics;

DX12IndexBuffer::DX12IndexBuffer(
	ID3D12Resource* a_resource,
	const DXGI_FORMAT a_indexFormat,
	const uint32_t a_vertexCount,
	const D3D12_RESOURCE_STATES a_initialState)
{
	m_view =
	{
		a_resource->GetGPUVirtualAddress(),
		(uint32_t)(FormatToSize(a_indexFormat) * a_vertexCount),
		a_indexFormat,
	};

	m_indexCount = a_vertexCount;
	m_resource = a_resource;
	m_currentState = a_initialState;
	m_initialized = 1;
}

const size_t DX12IndexBuffer::GetIndexSize() const
{
	return FormatToSize(m_view.Format);
}