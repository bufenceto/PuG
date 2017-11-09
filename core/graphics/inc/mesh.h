#pragma once
#include "vertex.h"

namespace pug
{
namespace graphics
{
	// Strong-typed vertex buffer handles (uint32_t)
	class VertexBufferHandle
	{
		VertexBufferHandle(uint32_t a_id)
			: m_id(a_id)
		{}

		~VertexBufferHandle() {}

		void operator= (VertexBufferHandle& other)
		{
			m_id = other.m_id;
		}



	private:
		VertexBufferHandle() = default;
		void operator= (uint32_t a_id)
		{
			m_id = a_id;
		}

		uint32_t m_id;
	};

	// Strong-typed index buffer handles (uint32_t)
	class IndexBufferHandle
	{
		IndexBufferHandle(uint32_t a_id)
			: m_id(a_id)
		{}

		~IndexBufferHandle() {}

		void operator= (IndexBufferHandle& other)
		{
			m_id = other.m_id;
		}



	private:
		IndexBufferHandle() = default;
		void operator= (uint32_t a_id)
		{
			m_id = a_id;
		}

		uint32_t m_id;
	};

	struct Mesh
	{
		VertexBufferHandle vbHandle;
		IndexBufferHandle ibHandle;

		uint32_t vertexCount;
		uint32_t indexCount;

		uint32_t startVertex;
		uint32_t startIndex;
	};
}
}
