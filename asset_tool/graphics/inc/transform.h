#pragma once
#include "vmath/vmath.h"
#include "matrix.h"

namespace pug {
namespace assets{
namespace graphics {

	struct Transform
	{
		vmath::Vector3 position;
		vmath::Quaternion rotation;
		vmath::Vector3 scale;
	};//40 bytes

	inline vmath::Vector3 GetLocalRight(const Transform& transform)
	{
		return RIGHT * transform.rotation;
	}
	inline vmath::Vector3 GetLocalUp(const Transform& transform)
	{
		return UP * transform.rotation;
	}
	inline vmath::Vector3 GetLocalForward(const Transform& transform)
	{
		return FORWARD * transform.rotation;
	}
	inline vmath::Matrix4 GetWorldMatrix(const Transform& transform)
	{
		vmath::Matrix4 scale = utility::CreateScaleMatrix(transform.scale);
		vmath::Matrix4 rotate = utility::CreateRotationMatrix(transform.rotation);
		vmath::Matrix4 translate = utility::CreateTranslateMatrix(transform.position);

		return scale * rotate * translate;
	}

}//vpl::assets::graphics
}//vpl::assets
}//vpl