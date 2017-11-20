#pragma once
#include "result_codes.h"
#include "handles.h"
#include "formats.h"
#include "transform.h"
#include "mesh.h"

#include "vmath/vmath.h"

#include <string>

namespace pug {
namespace platform{
	class Window;
}
namespace assets{
namespace graphics{

	struct Rect
	{
		vmath::Int2 leftTop;
		vmath::Int2 rightBottom;
	};

	PUG_RESULT InitGraphics(
		pug::platform::Window* a_window,
		uint32_t a_verticalSyncInterval,
		uint32_t a_fullscreen);
	PUG_RESULT DestroyGraphics();

	PUG_RESULT Render(
		const vmath::Matrix4& viewMatrix,
		const vmath::Matrix4& projectionMatrix,
		const vmath::Vector3& cameraPosition,
		const Transform* const a_transforms,
		const Mesh* const a_meshes,
		const uint32_t a_numMeshes,
		const graphics::Mesh* const a_guiMeshes,
		const graphics::Rect* const a_guiScissorRects,
		const Texture2DHandle* const a_guiTextures,
		const uint32_t a_guiMeshCount);

	PUG_RESULT CreateVertexBuffer(
		const void* a_data,
		const uint64_t a_vertexStride,
		const uint32_t vertexCount,
		VertexBufferHandle& out_result,
		const std::string& a_name = "");

	PUG_RESULT CreateIndexBuffer(
		const void* a_data,
		const PUG_FORMAT a_indexFormat,
		const uint32_t a_indexCount,
		IndexBufferHandle& out_result,
		const std::string& a_name = "");

	PUG_RESULT CreateTexture2D(
		void* a_data,
		uint32_t a_width,
		uint32_t a_height,
		PUG_FORMAT a_format,
		uint32_t a_mipCount,
		Texture2DHandle& out_texture,
		const std::string& a_name = "");

	PUG_RESULT UpdateVertexBuffer(
		const void* a_data,
		const size_t vertexStride,
		const size_t vertexCount,
		VertexBufferHandle& inout_vbh);
	PUG_RESULT UpdateIndexBuffer(
		const void* a_data,
		const PUG_FORMAT a_indexFormat,
		const uint32_t a_indexCount,
		IndexBufferHandle& inout_vbh);

	void DestroyVertexBuffer(
		VertexBufferHandle& vbh);
	void DestroyIndexBuffer(
		IndexBufferHandle& ibh);
	void DestroyTexture2D(
		Texture2DHandle& th);

	uint32_t IsVertexBufferValid(VertexBufferHandle& vbh);
	uint32_t IsIndexBufferValid(IndexBufferHandle& ibh);
	uint32_t IsTextureValid(Texture2DHandle& th);
}
}
}