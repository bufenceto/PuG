#pragma once
#include <cstdint>

#define DeclareHandle(CLASS)												\
class CLASS																	\
{																			\
public:																		\
	CLASS()																	\
		: m_index(0)														\
	{																		\
																			\
	}																		\
	CLASS(uint32_t a_index)													\
		: m_index(a_index){}												\
	~CLASS(){}																\
																			\
	CLASS(const CLASS& other)												\
	{																		\
		m_index = other.m_index;											\
	}																		\
	CLASS(CLASS&& other)													\
	{																		\
		m_index = other.m_index;											\
		other.m_index = 0;													\
	}																		\
																			\
	CLASS& operator=(const CLASS& other)									\
	{																		\
		m_index = other.m_index;											\
		return *this;														\
	}																		\
	CLASS& operator=(CLASS&& other)											\
	{																		\
		m_index = other.m_index;											\
		other.m_index = 0;													\
		return *this;														\
	}																		\
																			\
	friend bool operator==(const CLASS& handle, const uint32_t i)			\
	{																		\
		return handle.m_index == i;											\
	}																		\
																			\
	operator const uint32_t()	const										\
	{																		\
		return m_index ;													\
	}																		\
																			\
private:																	\
	uint32_t m_index;														\
}; 

namespace pug {
namespace assets {
namespace graphics {

DeclareHandle(Texture2DHandle);
DeclareHandle(VertexBufferHandle);
DeclareHandle(IndexBufferHandle);

}
}
}