#pragma once
#include "vmath/vmath.h"

#include "result_codes.h"

#include <string>

namespace pug{
namespace platform{

class Window
{
public:
	virtual ~Window() = 0 {};

	static Window* Create(const vmath::Int2& a_size, const std::string& a_title, const uint32_t& a_fullScreen);

	const vmath::Int2 GetSize() { return m_size; }
	const uint32_t IsMaximized() const { return m_maximized; }

	void SetSize(const vmath::Int2& a_size) { m_size = a_size; }

	virtual PUG_RESULT Update() = 0;

protected:
	uint32_t m_maximized;
	vmath::Int2 m_size;
};


}
}