#include "macro.h"
#include "dx12_resource.h"

#include "d3dx12.h"

using namespace pug;
using namespace pug::assets;
using namespace pug::assets::graphics;

D3D12_RESOURCE_BARRIER DX12Resource::Transition(D3D12_RESOURCE_STATES a_newState)
{
	PUG_ASSERT(m_currentState != a_newState, "Before and after states are equal!");

	///<todo> MORE ERROR CHECKING!

	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_resource, m_currentState, a_newState);
	m_currentState = a_newState;

	return barrier;
}