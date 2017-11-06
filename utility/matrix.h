#pragma once
#include "vmath/vmath.h"
#include <math.h>

#include <cstdio>

#ifdef near
#undef near
#endif
#ifdef far
#undef far
#endif

namespace pug {
namespace utility {

	inline vmath::Matrix4 CreateLookAtMatrix(
		const vmath::Vector3& lookAt)
	{
		vmath::Vector3 direction = lookAt;
		float d = fabsf(vmath::Dot(FORWARD, lookAt));
		vmath::Vector3 tmp = d < 0.9f ? FORWARD : UP;
		vmath::Vector3 right = vmath::Normalize(vmath::Cross(tmp, direction));
		vmath::Vector3 up = vmath::Normalize(vmath::Cross(direction, right));
		vmath::Matrix4 m = vmath::Matrix4(vmath::Vector4(right, 0.0f),
										  vmath::Vector4(up, 0.0f),
										  vmath::Vector4(direction, 0.0f),
										  vmath::Vector4(0.0f, 0.0f, 0.0f, 1.0f));
		return m;		
	}
	inline vmath::Matrix4 CreateViewMatrix(
		const vmath::Vector3& cameraPosition, 
		const vmath::Quaternion& rotation)
	{
		vmath::Matrix4 negTrans = vmath::Matrix4(vmath::Vector4(1, 0, 0, 0),
												 vmath::Vector4(0, 1, 0, 0),
												 vmath::Vector4(0, 0, 1, 0),
												 vmath::Vector4(-cameraPosition, 1));
		vmath::Matrix4 transRot = Transpose(vmath::QuaternionToMatrix(rotation));
		return negTrans * transRot;
	}
	inline vmath::Matrix4 CreateViewMatrix(
		const vmath::Vector3& cameraPosition, 
		const vmath::Matrix4& rotation)
	{
		vmath::Matrix4 negTrans = vmath::Matrix4(vmath::Vector4(1, 0, 0, 0),
												 vmath::Vector4(0, 1, 0, 0),
												 vmath::Vector4(0, 0, 1, 0),
												 vmath::Vector4(-cameraPosition, 1));
		vmath::Matrix4 transRot = Transpose(rotation);
		return negTrans * transRot;		
	}	
	inline vmath::Matrix4 CreateProjectionMatrix(
		float fov_radians, 
		float aspectRatio, 
		float zNear, 
		float zFar)
	{//LH
		vmath::Matrix4 m = vmath::Matrix4();
		float tanHalfFovy = tanf(fov_radians * 0.5f);
		m[0][0] = 1.0f / (aspectRatio * tanHalfFovy);
		m[1][1] = 1.0f / (tanHalfFovy);
		m[2][2] = (zFar) / (zFar - zNear);
		m[2][3] = 1.0f;//copy z value to w for perpsective divide
		m[3][2] = -zNear * zFar / (zFar - zNear);
		m[3][3] = 0.0f;
		return m;
	}
	inline vmath::Matrix4 CreateInfiniteProjectionMatrix(
		float fov_radians, 
		float aspectRatio, 
		float zNear)
	{
		vmath::Matrix4 m = vmath::Matrix4();
		float tanHalfFovy = tanf(fov_radians / 2.0f);
		m[0][0] = 1.0f / (aspectRatio * tanHalfFovy);
		m[1][1] = 1.0f / (tanHalfFovy);
		m[2][2] = 1.0f;//(zFar) / (zFar - zNear);
		m[2][3] = 1.0f;//copy z value to w for perpsective divide
		m[3][2] = -zNear;//(zFar * zNear) / (zFar - zNear);
		m[3][3] = 0.0f;
		return m;
	}
	inline vmath::Matrix4 CreateOffCenterOrthographicProjectionMatrix(
		float left, float right,
		float top, float bottom,
		float near, float far)
	{
		vmath::Matrix4 m;
		m[0][0] = 2.0f / (right - left);
		m[1][1] = 2.0f / (top - bottom);
		m[2][2] = 1.0f / (far - near);
		m[3][0] = (right + left) / (left - right);
		m[3][1] = (top + bottom) / (bottom - top);
		m[3][2] = -near / (far - near);
		return m;
		
		/*
		vmath::Matrix4 m;
		m[0][0] = 2.0f / (right - left);
		m[1][1] = 2.0f / (top - bottom);
		m[2][2] = 1.0f / (far - near);
		//m[2][3] = -near / (far - near);
		m[3][0] = (right + left) / (left - right);
		m[3][1] = (top + bottom) / (bottom - top);
		m[3][2] = 1.0f;
		return m;
		*/
	}
	inline vmath::Matrix4 CreateOrthographicProjectionMatrix(
		float width,
		float height,
		float near, 
		float far)
	{
		vmath::Matrix4 m;
		m[0][0] = 2.0f / width;
		m[1][1] = 2.0f / height;
		m[2][2] = 1.0f / (far - near);
		m[3][2] = -near / (far - near);
		return m;
	}
	inline vmath::Matrix4 CreateTranslateMatrix(
		const vmath::Vector3& translation)
	{
		return vmath::Matrix4(vmath::Vector4(1,0,0,0),
							  vmath::Vector4(0,1,0,0),
							  vmath::Vector4(0,0,1,0),
							  vmath::Vector4(translation,1));
	}
	inline vmath::Matrix4 CreateRotationMatrix(
		const vmath::Quaternion& rotation)
	{
		return vmath::QuaternionToMatrix(rotation);
	}
	inline vmath::Matrix4 CreateScaleMatrix(
		const vmath::Vector3& scale)
	{
		return vmath::Matrix4(scale.x, 0, 0, 0,
							  0, scale.y, 0, 0,
							  0, 0, scale.z, 0,
							  0, 0, 0, 1);
	}
	inline vmath::Matrix4 CreateDeltaMatrix(
		const vmath::Matrix4& a, //prev
		const vmath::Matrix4& b) //curr
	{
		return b * Inverse(a);
	}
						
}//pug::utility
}//pug
