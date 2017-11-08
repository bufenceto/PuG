#pragma once
#include <cstdint>
#include "vmath\vmath.h"
#include <string>
#include "result_codes.h"

namespace pug {
namespace graphics {
	class Window
	{
	public:
		Window() = default;
		~Window() {};

		static Window* Create(const std::string& a_title, const vmath::Int2& a_size);
		
		virtual void DispatchMessages() = 0;

		vmath::Int2 GetSize()
		{
			return m_size;
		}

		virtual void Destroy() = 0;

	protected:
		vmath::Int2 m_size;

	private:

	};

}
}