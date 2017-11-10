#pragma once
#include <cstdint>

#define DeclareHandle(CLASS)								\
class CLASS													\
{															\
public:														\
	CLASS()													\
		: m_index(0)										\
	{														\
															\
	}														\
	CLASS(uint32_t a_index)									\
		: m_index(a_index){}								\
	~CLASS(){}												\
															\
	CLASS(const CLASS& other)								\
	{														\
		m_index = other.m_index;							\
	}														\
	CLASS(CLASS&& other)									\
	{														\
		m_index = other.m_index;							\
		other.m_index = 0;									\
	}														\
															\
	CLASS& operator=(const CLASS& other)					\
	{														\
		m_index = other.m_index;							\
		return *this;										\
	}														\
	CLASS& operator=(CLASS&& other)							\
	{														\
		m_index = other.m_index;							\
		other.m_index = 0;									\
		return *this;										\
	}														\
															\
	operator uint32_t()										\
	{														\
		return m_index;										\
	}														\
															\
private:													\
	uint32_t m_index;										\
}; 

DeclareHandle(TextureHandle);
DeclareHandle(VertexBufferHandle);
DeclareHandle(IndexBufferHandle);