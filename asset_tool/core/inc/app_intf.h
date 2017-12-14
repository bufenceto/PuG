#pragma once
#include "vmath/vmath.h"

#include <cstdint>

namespace pug{
namespace assets {

	float GetDeltaTime();
	void Stop();

namespace graphics {

	struct Mesh;
	struct Camera;
	struct Transform;
	struct Material;

	struct PointLight;
	struct DirectionalLight;

}
}//vpl::graphics
	
	class iApp
	{
	public:
		virtual ~iApp() = 0 {}

		static iApp* Create();

		virtual bool Initialize() = 0;
		virtual bool Destroy() = 0;

		virtual bool Update(
			const float deltaTime) = 0;

		virtual void GetRenderData(
			pug::assets::graphics::Mesh*& out_meshes,
			pug::assets::graphics::Transform*& out_transforms,
			pug::assets::graphics::Material*& out_materials,
			uint32_t& out_meshCount) = 0;
		virtual void GetMainCamera(
			pug::assets::graphics::Camera& out_camera,
			vmath::Vector3& out_position,
			vmath::Quaternion& out_rotation) = 0;
		virtual void GetLightData(
			pug::assets::graphics::PointLight*& out_pointLights,
			uint32_t& out_pointLightCount,
			pug::assets::graphics::DirectionalLight*& out_directionalLights,
			vmath::Vector4*& out_directionalLightShadowMapBoxes,
			uint32_t& out_directionalLightCount) = 0;
	};

}