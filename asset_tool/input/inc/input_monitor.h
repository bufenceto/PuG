#pragma once
#include "vmath/vmath.h"
#include "macro.h"
#include "key_codes.h"
//#include "vorpal_typedef.h"

namespace pug{
namespace input{
	
	struct InputMessage;

	PUG_RESULT InitializeInput(const uint32_t& windowMaximized, const vmath::Int2& windowSize);
	PUG_RESULT ResolveInput();
	PUG_RESULT SubmitInputMessage(const InputMessage& message);

}// pug::input
}// pug