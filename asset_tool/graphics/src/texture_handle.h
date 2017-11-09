#pragma once
#include <cstdint>

#define DeclareHandle(name)									\
class name													\
{															\
public:														\
	name(uint32_t a_index)									\
		: m_index(a_index){}								\
	~name(){}												\
															\
	name(const name& other)									\
	{														\
		m_index = other.m_index;							\
	}														\
	name(name&& other)										\
	{														\
		m_index = other.m_index;							\
		other.m_index = 0;									\
	}														\
															\
	name& operator=(const name& other)						\
	{														\
		m_index = other.m_index;							\
		return *this;										\
	}														\
	name& operator=(name&& other)							\
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

DeclareHandle(TextureHandle)
DeclareHandle(MeshHandle)