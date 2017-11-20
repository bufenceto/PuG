#include "dx12_persistent_descriptor_heap.h"

#include "resource/dx12_texture2D.h"
#include "defines.h"

#include "logger.h"

#include <d3d12.h>

#define DESCRIPTORS_FREE 0x0
#define DESCRIPTOR_TAKEN 0x1

namespace pug{
namespace assets{
namespace graphics {

	class DescriptorPage
	{
	public:
		DescriptorPage()
			: m_heap(nullptr)
			, m_flagArray(nullptr)
			, m_maxDescriptors(0)
			, m_handleIncrement(0)
		{}
		DescriptorPage(
			D3D12_DESCRIPTOR_HEAP_TYPE type,
			uint32_t maxDescriptors,
			ID3D12Device* device,
			bool shaderVisible)
		{
			D3D12_DESCRIPTOR_HEAP_DESC desc = {};
			desc.Flags = (shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
			desc.NodeMask = 0;
			desc.NumDescriptors = maxDescriptors;
			desc.Type = type;

			HRESULT hr;
			hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_heap));
			if (FAILED(hr))
			{
				log::Error("Failed to initialize descriptor heap");
				return;
			}

			m_flagArray = (uint8_t*)_aligned_malloc(sizeof(uint8_t) * maxDescriptors, 16);
			memset(m_flagArray, 0, sizeof(uint8_t) * maxDescriptors);

			m_handleIncrement = device->GetDescriptorHandleIncrementSize(type);
			m_maxDescriptors = maxDescriptors;
		}
		DescriptorPage(const DescriptorPage& other)
		{
			m_heap = other.m_heap;
			memcpy(m_flagArray, other.m_flagArray, sizeof(uint8_t) * other.m_maxDescriptors);
			m_maxDescriptors = other.m_maxDescriptors;// 4 bytes
			m_handleIncrement = other.m_handleIncrement;// 4 bytes
		}
		DescriptorPage(DescriptorPage&& other)
		{
			m_heap = other.m_heap;// 8 bytes
			m_flagArray = other.m_flagArray;// 8 bytes
			m_maxDescriptors = other.m_maxDescriptors;// 4 bytes
			m_handleIncrement = other.m_handleIncrement;// 4 bytes

			other.m_heap = nullptr;
			other.m_flagArray = nullptr;
			other.m_maxDescriptors = 0;
			other.m_handleIncrement = 0;
		}
		~DescriptorPage()
		{
			m_handleIncrement = 0;
			if (m_heap != nullptr)
			{
				m_heap->Release();
				m_heap = nullptr;
			}
			if (m_flagArray != nullptr)
			{
				_aligned_free(m_flagArray);
				m_flagArray = nullptr;
			}
		}

		DescriptorPage& operator=(const DescriptorPage& other)
		{
			m_heap = other.m_heap;
			memcpy(m_flagArray, other.m_flagArray, sizeof(uint8_t) * other.m_maxDescriptors);
			m_maxDescriptors = other.m_maxDescriptors;// 4 bytes
			m_handleIncrement = other.m_handleIncrement;// 4 bytes

			return *this;
		}
		DescriptorPage& operator=(DescriptorPage&& other)
		{
			m_heap = other.m_heap;// 8 bytes
			m_flagArray = other.m_flagArray;// 8 bytes
			m_maxDescriptors = other.m_maxDescriptors;// 4 bytes
			m_handleIncrement = other.m_handleIncrement;// 4 bytes

			other.m_heap = nullptr;
			other.m_flagArray = nullptr;
			other.m_maxDescriptors = 0;
			other.m_handleIncrement = 0;

			return *this;
		}

		PUG_RESULT AllocateDescriptors(
			//D3D12_CPU_DESCRIPTOR_HANDLE& out_cpuDescriptor,
			//D3D12_GPU_DESCRIPTOR_HANDLE& out_gpuDescriptor,
			uint32_t& out_descriptorIndex)
		{
			for (uint32_t i = 0; i < m_maxDescriptors; ++i)
			{
				if (i != PUG_INVALID_ID && m_flagArray[i] == DESCRIPTORS_FREE)
				{//this slot is free
					m_flagArray[i] = DESCRIPTOR_TAKEN;//flag as taken

					//D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptor = m_heap->GetCPUDescriptorHandleForHeapStart();
					//cpuDescriptor.ptr += (i * m_handleIncrement);
					//out_cpuDescriptor = cpuDescriptor;

					//D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptor = m_heap->GetGPUDescriptorHandleForHeapStart();
					//gpuDescriptor.ptr += (i * m_handleIncrement);
					//out_gpuDescriptor = gpuDescriptor;

					out_descriptorIndex = i;

					return PUG_RESULT_OK;
				}
			}
			return PUG_RESULT_ALLOCATION_FAILURE;
		}
		PUG_RESULT ReleaseDescriptors(
			const uint32_t& descriptorIndex)
		{
			PUG_ASSERT(descriptorIndex < m_maxDescriptors, "index exceeds heap size");

			if (m_flagArray[descriptorIndex] == DESCRIPTOR_TAKEN)
			{
				m_flagArray[descriptorIndex] = DESCRIPTORS_FREE;
			}
			else
			{
				log::Warning("The descriptor slot was not marked as taken!");
			}

			return PUG_RESULT_OK;
		}

		uint32_t GetMaxDescriptorsPerPage() const
		{
			return m_maxDescriptors;
		}
		uint32_t IsEmpty()
		{
			for (uint32_t i = 0; i < m_maxDescriptors; ++i)
			{
				if (m_flagArray[i] == DESCRIPTOR_TAKEN)
				{//this slot is taken
					return 0;
				}
			}
			return 1;
		}

		D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptor(const uint32_t a_descriptorIndex)
		{
			D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptor = m_heap->GetCPUDescriptorHandleForHeapStart();
			cpuDescriptor.ptr += (a_descriptorIndex * m_handleIncrement);
			return cpuDescriptor;
		}
		D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptor(const uint32_t a_descriptorIndex)
		{
			D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptor = m_heap->GetGPUDescriptorHandleForHeapStart();
			gpuDescriptor.ptr += (a_descriptorIndex * m_handleIncrement);
			return gpuDescriptor;
		}

		ID3D12DescriptorHeap* const GetHeap() const
		{
			return m_heap;
		}

	private:
		ID3D12DescriptorHeap* m_heap;// 8 bytes
		uint8_t* m_flagArray;// 8 bytes
		uint32_t m_maxDescriptors;// 4 bytes
		uint32_t m_handleIncrement;// 4 bytes
	};//24 bytes

}
}
}

using namespace pug;
using namespace pug::assets;
using namespace pug::assets::graphics;

static std::vector<DescriptorPage> m_srvHeaps;
static std::vector<DescriptorPage> m_dsvHeaps;
static std::vector<DescriptorPage> m_rtvHeaps;
static std::vector<DescriptorPage> m_uavHeaps;
static std::vector<DescriptorPage> m_nsvUAVHeaps;

static uint32_t m_srvHeapCapacity;
static uint32_t m_rtvHeapCapacity;
static uint32_t m_dsvHeapCapacity;
static uint32_t m_uavHeapCapacity;
static uint32_t m_nsvUAVHeapCapacity;

ID3D12Device* m_device;

void AddSRVHeap();
void AddDSVHeap();
void AddRTVHeap();
void AddUAVHeap();
void AddnsvUAVHeap();

void AddSRVHeap()
{
	m_srvHeaps.push_back(
		DescriptorPage(
			D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			m_srvHeapCapacity,
			m_device,
			true));
}

void AddDSVHeap()
{
	m_dsvHeaps.push_back(
		DescriptorPage(
			D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
			m_dsvHeapCapacity,
			m_device,
			false));
}

void AddRTVHeap()
{
	m_rtvHeaps.push_back(
		DescriptorPage(
			D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
			m_rtvHeapCapacity,
			m_device,
			false));
}

void AddUAVHeap()
{
	m_uavHeaps.push_back(
		DescriptorPage(
			D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			m_uavHeapCapacity,
			m_device,
			true));
}

void AddnsvUAVHeap()
{
	m_nsvUAVHeaps.push_back(
		DescriptorPage(
			D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			m_uavHeapCapacity,
			m_device,
			false));
}

PUG_RESULT pug::assets::graphics::InitPersistentDescriptorHeap(
	ID3D12Device* a_device,
	uint32_t a_srvHeapCapacity,
	uint32_t a_dsvHeapCapacity,
	uint32_t a_rtvHeapCapacity,
	uint32_t a_uavHeapCapacity,
	uint32_t a_nsvUAVHeapCapacity)
{
	m_device = a_device;

	m_srvHeapCapacity = a_srvHeapCapacity;
	m_rtvHeapCapacity = a_dsvHeapCapacity;
	m_dsvHeapCapacity = a_rtvHeapCapacity;
	m_uavHeapCapacity = a_uavHeapCapacity;
	m_nsvUAVHeapCapacity = a_nsvUAVHeapCapacity;
	
	AddSRVHeap();
	AddDSVHeap();
	AddRTVHeap();
	AddUAVHeap();
	AddnsvUAVHeap();

	return PUG_RESULT_OK;
}

uint32_t pug::assets::graphics::DestroyPersistentDescriptorHeap()
{
	for (uint32_t i = 0; i < m_srvHeaps.size(); ++i)
	{
		DescriptorPage& page = m_srvHeaps[i];
		if (!page.IsEmpty())
		{
			log::Warning("Destroying committed resource heap but a SRV page with index %d was not empty!", i);
		}
	}
	m_srvHeaps.clear();
	for (uint32_t i = 0; i < m_rtvHeaps.size(); ++i)
	{
		DescriptorPage& page = m_rtvHeaps[i];
		if (!page.IsEmpty())
		{
			log::Warning("Destroying committed resource heap but a RTV page with index %d was not empty!", i);
		}
	}
	m_rtvHeaps.clear();
	for (uint32_t i = 0; i < m_dsvHeaps.size(); ++i)
	{
		DescriptorPage& page = m_dsvHeaps[i];
		if (!page.IsEmpty())
		{
			log::Warning("Destroying committed resource heap but a DSV page with index %d was not empty!", i);
		}
	}
	m_dsvHeaps.clear();
	for (uint32_t i = 0; i < m_uavHeaps.size(); ++i)
	{
		DescriptorPage& page = m_uavHeaps[i];
		if (!page.IsEmpty())
		{
			log::Warning("Destroying committed resource heap but a UAV page with index %d was not empty!", i);
		}
	}
	m_uavHeaps.clear();
	for (uint32_t i = 0; i < m_nsvUAVHeaps.size(); ++i)
	{
		DescriptorPage& page = m_nsvUAVHeaps[i];
		if (!page.IsEmpty())
		{
			log::Warning("Destroying committed resource heap but a non shader visible UAV page with index %d was not empty!", i);
		}
	}
	m_nsvUAVHeaps.clear();

	m_srvHeapCapacity = 0;
	m_rtvHeapCapacity = 0;
	m_dsvHeapCapacity = 0;
	m_uavHeapCapacity = 0;
	m_nsvUAVHeapCapacity = 0;

	m_device = nullptr;

	return PUG_RESULT_OK;
}

uint32_t pug::assets::graphics::AllocatePersistentSRVDescriptors()
{
	PUG_RESULT res = PUG_RESULT_UNKNOWN;
	uint32_t pageIndex = 0;
	uint32_t descriptorIndex = 0;
	do
	{
		res = m_srvHeaps[pageIndex].AllocateDescriptors(/*out_cpuDescriptor, out_gpuDescriptor,*/ descriptorIndex);
		if (res == PUG_RESULT_ALLOCATION_FAILURE)
		{//if the heap reported that its page is full we add a new heap and allocate from there
			++pageIndex;
			if (pageIndex == m_srvHeaps.size())
			{
				AddSRVHeap();
			}
		}
	} while (res != PUG_RESULT_OK);

	return (pageIndex * m_srvHeaps[pageIndex].GetMaxDescriptorsPerPage()) + descriptorIndex;
}

uint32_t pug::assets::graphics::AllocatePersistentRTVDescriptors()
{
	PUG_RESULT res = PUG_RESULT_UNKNOWN;
	uint32_t pageIndex = 0;
	uint32_t descriptorIndex = 0;
	do
	{
		res = m_rtvHeaps[pageIndex].AllocateDescriptors(/*out_cpuDescriptor, out_gpuDescriptor,*/ descriptorIndex);
		if (res == PUG_RESULT_ALLOCATION_FAILURE)
		{//if the heap reported that its page is full we add a new heap and allocate from there
			++pageIndex;
			if (pageIndex == m_rtvHeaps.size())
			{
				AddRTVHeap();
			}
		}
	} while (res != PUG_RESULT_OK);

	return pageIndex * m_rtvHeaps[pageIndex].GetMaxDescriptorsPerPage() + descriptorIndex;
}

uint32_t pug::assets::graphics::AllocatePersistentDSVDescriptors()
{
	PUG_RESULT res = PUG_RESULT_UNKNOWN;
	uint32_t pageIndex = 0;
	uint32_t descriptorIndex = 0;
	do
	{
		res = m_dsvHeaps[pageIndex].AllocateDescriptors(/*out_cpuDescriptor, out_gpuDescriptor,*/ descriptorIndex);
		if (res == PUG_RESULT_ALLOCATION_FAILURE)
		{//if the heap reported that its page is full we add a new heap and allocate from there
			++pageIndex;
			if (pageIndex == m_dsvHeaps.size())
			{
				AddDSVHeap();
			}
		}
	} while (res != PUG_RESULT_OK);

	return pageIndex * m_dsvHeaps[pageIndex].GetMaxDescriptorsPerPage() + descriptorIndex;
}

uint32_t pug::assets::graphics::AllocatePersistentUAVDescriptors()
{
	PUG_RESULT res = PUG_RESULT_UNKNOWN;
	uint32_t pageIndex = 0;
	uint32_t descriptorIndex = 0;
	do
	{
		res = m_uavHeaps[pageIndex].AllocateDescriptors(/*out_cpuDescriptor, out_gpuDescriptor,*/ descriptorIndex);
		if (res == PUG_RESULT_ALLOCATION_FAILURE)
		{//if the heap reported that its page is full we add a new heap and allocate from there
			++pageIndex;
			if (pageIndex == m_uavHeaps.size())
			{
				AddUAVHeap();
			}
		}
	} while (res != PUG_RESULT_OK);

	return pageIndex * m_uavHeaps[pageIndex].GetMaxDescriptorsPerPage() + descriptorIndex;
}

uint32_t pug::assets::graphics::AllocatePersistentNSVUAVDescriptors()
{
	PUG_RESULT res = PUG_RESULT_UNKNOWN;
	uint32_t pageIndex = 0;
	uint32_t descriptorIndex = 0;
	do
	{
		res = m_nsvUAVHeaps[pageIndex].AllocateDescriptors(/*out_cpuDescriptor, out_gpuDescriptor,*/ descriptorIndex);
		if (res == PUG_RESULT_ALLOCATION_FAILURE)
		{//if the heap reported that its page is full we add a new heap and allocate from there
			++pageIndex;
			if (pageIndex == m_nsvUAVHeaps.size())
			{
				AddnsvUAVHeap();
			}
		}
	} while (res != PUG_RESULT_OK);

	return pageIndex * m_nsvUAVHeaps[pageIndex].GetMaxDescriptorsPerPage() + descriptorIndex;
}

PUG_RESULT pug::assets::graphics::ReleasePersistentSRVDescriptors(
	const uint32_t& inout_heapIndex)
{
	uint32_t pageSize = m_srvHeaps[0].GetMaxDescriptorsPerPage();

	if (inout_heapIndex != PUG_INVALID_ID)
	{
		uint32_t pageIndex = inout_heapIndex / pageSize;
		uint32_t descriptorIndex = inout_heapIndex % pageSize;

		m_srvHeaps[pageIndex].ReleaseDescriptors(descriptorIndex);
	}

	return PUG_RESULT_OK;
}

PUG_RESULT pug::assets::graphics::ReleasePersistentRTVDescriptors(
	const uint32_t& inout_heapIndex)
{
	uint32_t pageSize = m_rtvHeaps[0].GetMaxDescriptorsPerPage();

	if (inout_heapIndex != PUG_INVALID_ID)
	{
		uint32_t pageIndex = inout_heapIndex / pageSize;
		uint32_t descriptorIndex = inout_heapIndex % pageSize;

		m_rtvHeaps[pageIndex].ReleaseDescriptors(descriptorIndex);
	}

	return PUG_RESULT_OK;
}

PUG_RESULT pug::assets::graphics::ReleasePersistentDSVDescriptors(
	const uint32_t& inout_heapIndex)
{
	uint32_t pageSize = m_dsvHeaps[0].GetMaxDescriptorsPerPage();

	if (inout_heapIndex != PUG_INVALID_ID)
	{
		uint32_t pageIndex = inout_heapIndex / pageSize;
		uint32_t descriptorIndex = inout_heapIndex % pageSize;

		m_dsvHeaps[pageIndex].ReleaseDescriptors(descriptorIndex);
	}

	return PUG_RESULT_OK;
}

PUG_RESULT pug::assets::graphics::ReleasePersistentUAVDescriptors(
	const uint32_t& inout_heapIndex)
{
	uint32_t pageSize = m_uavHeaps[0].GetMaxDescriptorsPerPage();

	if (inout_heapIndex != PUG_INVALID_ID)
	{
		uint32_t pageIndex = inout_heapIndex / pageSize;
		uint32_t descriptorIndex = inout_heapIndex % pageSize;

		m_uavHeaps[pageIndex].ReleaseDescriptors(descriptorIndex);
	}

	return PUG_RESULT_OK;
}

PUG_RESULT pug::assets::graphics::ReleasePersistentNSVUAVDescriptors(
	const uint32_t& inout_heapIndex)
{
	uint32_t pageSize = m_nsvUAVHeaps[0].GetMaxDescriptorsPerPage();

	if (inout_heapIndex != PUG_INVALID_ID)
	{
		uint32_t pageIndex = inout_heapIndex / pageSize;
		uint32_t descriptorIndex = inout_heapIndex % pageSize;

		m_nsvUAVHeaps[pageIndex].ReleaseDescriptors(descriptorIndex);
	}

	return PUG_RESULT_OK;
}

PUG_RESULT pug::assets::graphics::GetPersistentSRVDescriptors(
	const uint32_t& srvDescriptorIndex,
	D3D12_CPU_DESCRIPTOR_HANDLE* out_cpuHandle,
	D3D12_GPU_DESCRIPTOR_HANDLE* out_gpuHandle)
{
	uint32_t pageIndex = srvDescriptorIndex / m_srvHeaps[0].GetMaxDescriptorsPerPage();
	uint32_t descriptorIndex = srvDescriptorIndex % m_srvHeaps[0].GetMaxDescriptorsPerPage();

	PUG_ASSERT(pageIndex < m_srvHeaps.size(), "Page index out of bounds");

	if (out_cpuHandle)
	{
		*out_cpuHandle = m_srvHeaps[pageIndex].GetCPUDescriptor(descriptorIndex);
	}
	if (out_gpuHandle)
	{
		*out_gpuHandle = m_srvHeaps[pageIndex].GetGPUDescriptor(descriptorIndex);
	}

	return PUG_RESULT_OK;
}

PUG_RESULT pug::assets::graphics::GetPersistentRTVDescriptors(
	const uint32_t& rtvDescriptorIndex,
	D3D12_CPU_DESCRIPTOR_HANDLE* out_cpuHandle,
	D3D12_GPU_DESCRIPTOR_HANDLE* out_gpuHandle)
{
	uint32_t pageIndex = rtvDescriptorIndex / m_rtvHeaps[0].GetMaxDescriptorsPerPage();
	uint32_t descriptorIndex = rtvDescriptorIndex % m_rtvHeaps[0].GetMaxDescriptorsPerPage();

	PUG_ASSERT(pageIndex < m_rtvHeaps.size(), "Page index out of bounds");

	if (out_cpuHandle)
	{
		*out_cpuHandle = m_rtvHeaps[pageIndex].GetCPUDescriptor(descriptorIndex);
	}
	if (out_gpuHandle)
	{
		*out_gpuHandle = m_rtvHeaps[pageIndex].GetGPUDescriptor(descriptorIndex);
	}

	return PUG_RESULT_OK;
}

PUG_RESULT pug::assets::graphics::GetPersistentDSVDescriptors(
	const uint32_t& dsvDescriptorIndex,
	D3D12_CPU_DESCRIPTOR_HANDLE* out_cpuHandle,
	D3D12_GPU_DESCRIPTOR_HANDLE* out_gpuHandle)
{
	uint32_t pageIndex = dsvDescriptorIndex / m_dsvHeaps[0].GetMaxDescriptorsPerPage();
	uint32_t descriptorIndex = dsvDescriptorIndex % m_dsvHeaps[0].GetMaxDescriptorsPerPage();

	PUG_ASSERT(pageIndex < m_dsvHeaps.size(), "Page index out of bounds");

	if (out_cpuHandle)
	{
		*out_cpuHandle = m_dsvHeaps[pageIndex].GetCPUDescriptor(descriptorIndex);
	}
	if (out_gpuHandle)
	{
		*out_gpuHandle = m_dsvHeaps[pageIndex].GetGPUDescriptor(descriptorIndex);
	}
	
	

	return PUG_RESULT_OK;
}

PUG_RESULT pug::assets::graphics::GetPersistentUAVDescriptors(
	const uint32_t& uavDescriptorIndex,
	D3D12_CPU_DESCRIPTOR_HANDLE* out_cpuHandle,
	D3D12_GPU_DESCRIPTOR_HANDLE* out_gpuHandle)
{
	uint32_t pageIndex = uavDescriptorIndex / m_uavHeaps[0].GetMaxDescriptorsPerPage();
	uint32_t descriptorIndex = uavDescriptorIndex % m_uavHeaps[0].GetMaxDescriptorsPerPage();

	PUG_ASSERT(pageIndex < m_uavHeaps.size(), "Page index out of bounds");

	if (out_cpuHandle)
	{
		*out_cpuHandle = m_uavHeaps[pageIndex].GetCPUDescriptor(descriptorIndex);
	}
	if (out_gpuHandle)
	{
		*out_gpuHandle = m_uavHeaps[pageIndex].GetGPUDescriptor(descriptorIndex);
	}
	

	return PUG_RESULT_OK;
}

PUG_RESULT pug::assets::graphics::GetPersistentNSVUAVDescriptors(
	const uint32_t& nsvUAVDescriptorIndex,
	D3D12_CPU_DESCRIPTOR_HANDLE* out_cpuHandle,
	D3D12_GPU_DESCRIPTOR_HANDLE* out_gpuHandle)
{
	uint32_t pageIndex = nsvUAVDescriptorIndex / m_nsvUAVHeaps[0].GetMaxDescriptorsPerPage();
	uint32_t descriptorIndex = nsvUAVDescriptorIndex % m_nsvUAVHeaps[0].GetMaxDescriptorsPerPage();

	PUG_ASSERT(pageIndex < m_nsvUAVHeaps.size(), "Page index out of bounds");

	if (out_cpuHandle)
	{
		*out_cpuHandle = m_nsvUAVHeaps[pageIndex].GetCPUDescriptor(descriptorIndex);
	}
	if (out_gpuHandle)
	{
		*out_gpuHandle = m_nsvUAVHeaps[pageIndex].GetGPUDescriptor(descriptorIndex);
	}

	return PUG_RESULT_OK;
}

ID3D12DescriptorHeap* const pug::assets::graphics::GetPersistentSRVDescriptorHeap(
	const uint32_t& srvDescriptorIndex)
{
	uint32_t pageIndex = srvDescriptorIndex / m_srvHeaps[0].GetMaxDescriptorsPerPage();
	return m_srvHeaps[pageIndex].GetHeap();
}