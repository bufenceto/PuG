#pragma once
#include "vmath.h"
#include "key_codes.h"
#include "result_codes.h"

namespace pug{
namespace input{

	typedef void(*InputCallBack)();

	vmath::Int2 GetMousePosition();
	vmath::Int2 GetMouseDelta();

	bool GetKey(EKeyID keyID);
	bool GetButton(EButtonID buttonID);
	const vmath::Int2& GetWindowSize();

	PUG_RESULT RegisterKeyEvent(const EKeyID& key, const EInputEvent& event, const InputCallBack& callback);
	PUG_RESULT RegisterButtonEvent(const EButtonID& button, const EInputEvent& event, const InputCallBack& callback);
	PUG_RESULT RegisterQuitEvent(const InputCallBack& callback);


}
}