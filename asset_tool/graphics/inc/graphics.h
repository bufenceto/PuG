#pragma once
#include "result_codes.h"
#include "handles.h"
#include "formats.h"

#include "vmath.h"

namespace pug {
namespace platform{
	class Window;
}
namespace assets{
namespace graphics{

	PUG_RESULT InitGraphics(
		pug::platform::Window* a_window,
		uint32_t a_verticalSyncInterval,
		uint32_t a_fullscreen);
	PUG_RESULT DestroyGraphics();

	PUG_RESULT Render();

	PUG_RESULT CreateVertexBuffer(
		const void* a_data,
		const uint64_t a_vertexStride,
		const uint32_t vertexCount,
		VertexBufferHandle& out_result);
	PUG_RESULT CreateIndexBuffer(
		const void* a_data,
		const PUG_FORMAT a_indexFormat,
		const uint32_t a_indexCount,
		IndexBufferHandle& out_result);

	PUG_RESULT CreateTexture2D();
}
}
}