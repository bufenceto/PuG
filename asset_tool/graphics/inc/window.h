#pragma once
#include "vmath.h"

#include "result_codes.h"

#include <string>

namespace pug{
namespace platform{

class Window
{
public:
	virtual ~Window() = 0 {};

	static Window* Create(const vmath::Int2& a_size, const std::string& a_title);

	const vmath::Int2 GetSize() { return m_size; }

	virtual PUG_RESULT Update() = 0;

protected:
	vmath::Int2 m_size;
};


}
}