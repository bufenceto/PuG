#pragma once
#include <cstdint>

namespace pug {
namespace graphics {

	//Mesh handles------------------------------------------------------------------------------------
	class MeshHandle
	{
		MeshHandle(uint32_t a_id)
			: m_id(a_id)
		{}

		~MeshHandle() {}

		void operator= (MeshHandle& other)
		{
			m_id = other.m_id;
		}



	private:
		MeshHandle() = default;
		void operator= (uint32_t a_id)
		{
			m_id = a_id;
		}

		uint32_t m_id;
	};

	//Texture handles---------------------------------------------------------------------------------
	class TextureHandle
	{
		TextureHandle(uint32_t a_id)
			: m_id(a_id)
		{}

		~TextureHandle() {}

		void operator= (TextureHandle& other)
		{
			m_id = other.m_id;
		}



	private:
		TextureHandle() = default;
		void operator= (uint32_t a_id)
		{
			m_id = a_id;
		}

		uint32_t m_id;
	};

}
}