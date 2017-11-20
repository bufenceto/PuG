#pragma once
#include "vmath/vmath.h"
#include "key_codes.h"
#include "result_codes.h"

namespace pug{
namespace input{

	typedef void(*InputCallBack)();

	vmath::Int2 GetMousePosition();
	vmath::Int2 GetMouseDelta();

	bool GetKey(KeyID keyID);
	bool GetButton(ButtonID buttonID);
	const vmath::Int2& GetWindowSize();

	PUG_RESULT RegisterKeyEvent(const KeyID& key, const InputEvent& event, const InputCallBack& callback);
	PUG_RESULT RegisterButtonEvent(const ButtonID& button, const InputEvent& event, const InputCallBack& callback);
	PUG_RESULT RegisterQuitEvent(const InputCallBack& callback);


}
}