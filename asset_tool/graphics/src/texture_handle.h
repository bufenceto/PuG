#pragma once
#include <cstdint>

class TextureHandle
{
public:
	TextureHandle(uint32_t a_index)
		: m_index(a_index){}
	~TextureHandle(){}

	TextureHandle(const TextureHandle& other)//copy
	{
		m_index = other.m_index;
	}
	TextureHandle(TextureHandle&& other)//move
	{
		m_index = other.m_index;
		other.m_index = 0;
	}

	TextureHandle& operator=(const TextureHandle& other)//copy
	{
		m_index = other.m_index;
		return *this;
	}
	TextureHandle& operator=(TextureHandle&& other)//move
	{
		m_index = other.m_index;
		other.m_index = 0;
		return *this;
	}

	uint32_t operator()()
	{
		return m_index;
	}

	//uint32_t operator=(const TextureHandle handle)
	//{
	//	return handle.m_index;
	//}

private:
	uint32_t m_index;
};