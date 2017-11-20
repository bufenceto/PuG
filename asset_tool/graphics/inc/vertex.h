#pragma once
#include "vmath/vmath.h"

namespace pug {
namespace assets{
namespace graphics {

	//PODs
	struct Vertex
	{
		vmath::Vector3 position;
		vmath::Vector3 normal;
		vmath::Vector3 tangent;
		vmath::Vector2 uv;
	};//44bytes

	struct GUIVertex
	{
		vmath::Vector2 position;
		vmath::Vector2 uv;
		uint32_t color;
	};//20 bytes

}//vpl::graphics
}
}//vpl