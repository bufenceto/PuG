#include "dx12_texture2D.h"

using namespace pug;
using namespace pug::assets;
using namespace pug::assets::graphics;

DX12Texture2D::DX12Texture2D(
	ID3D12Resource* a_resource,
	uint32_t a_srvHeapIndex,
	uint32_t a_dsvHeapIndex,
	uint32_t a_rtvHeapIndex,
	uint32_t a_uavHeapIndex,
	uint32_t a_nsvUAVHeapIndex,
	uint32_t a_width,
	uint32_t a_height,
	DXGI_FORMAT a_format,
	D3D12_RESOURCE_STATES a_initialState)
{
	m_resource = a_resource;
	m_srvHeapIndex = a_srvHeapIndex;
	m_dsvHeapIndex = a_dsvHeapIndex;
	m_rtvHeapIndex = a_rtvHeapIndex;
	m_uavHeapIndex = a_uavHeapIndex;
	m_nsvUAVHeapIndex = a_nsvUAVHeapIndex;
	m_width = a_width;
	m_height = a_height;
	m_format = a_format;
	m_currentState = a_initialState;
}

DX12Texture2D::~DX12Texture2D()
{

}