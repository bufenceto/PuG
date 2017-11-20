#include "gui.h"

#ifdef PUG_WINDOWS
#include "windows_window.h"
#else
#include "window.h"
#endif

#include "graphics.h"
#include "mesh.h"
#include "input.h"
#include "defines.h"

#include "imgui/imgui.h"
#include "vmath/vmath.h"

#define MAX_GUI_MESHES 16

using namespace vmath;
using namespace pug;
using namespace pug::assets;
using namespace pug::assets::graphics;
using namespace pug::platform;
using namespace pug::input;
/*
#define RED_MASK   0xFF000000
#define GREEN_MASK 0x00FF0000
#define BLUE_MASK  0x0000FF00
#define ALPHA_MASK 0x000000FF
*/

#define RED_MASK   0x000000FF
#define GREEN_MASK 0x0000FF00
#define BLUE_MASK  0x00FF0000
#define ALPHA_MASK 0xFF000000

/*
#define RED_SHIFT 24
#define GREEN_SHIFT 16
#define BLUE_SHIFT 8
#define ALPHA_SHIFT 0
*/

#define RED_SHIFT 0
#define GREEN_SHIFT 8
#define BLUE_SHIFT 16
#define ALPHA_SHIFT 24

static const PUG_FORMAT GetIndexFormatFromSize(const size_t a_size)
{
	switch (a_size)
	{
	case 1: return (PUG_FORMAT_R8_UINT);
	case 2: return (PUG_FORMAT_R16_UINT);
	case 4: return (PUG_FORMAT_R32_UINT);
	default: log::Warning("size not found!"); return PUG_FORMAT_NONE;
	}
}

//worst case we have a index buffer for every vertex buffer
static VertexBufferHandle g_vertexBuffers[MAX_GUI_MESHES];
static IndexBufferHandle g_indexBuffers[MAX_GUI_MESHES];

static Texture2DHandle g_fontTexture;

static Mesh g_meshes[MAX_GUI_MESHES];
static Rect g_scissorRects[MAX_GUI_MESHES];
static Texture2DHandle g_fontTextures[MAX_GUI_MESHES];

static bool g_guiHovered;

PUG_RESULT pug::assets::gui::InitGUI(Window* window)
{
	ImGuiIO& io = ImGui::GetIO();
	vmath::Int2 windowSize = window->GetSize();
	io.DisplaySize.x = (float)windowSize.x;
	io.DisplaySize.y = (float)windowSize.y;
	//io.IniFilename = "imgui.ini";
	//io.RenderDrawListsFn = renderFunc;  // Setup a render function, or set to NULL and call GetDrawData() after Render() to access the render data.
#ifdef PUG_WINDOWS
	io.ImeWindowHandle = dynamic_cast<windows::WindowsWindow*>(window)->GetWindowHandle();											// TODO: Fill others settings of the io structure
#endif
	
	unsigned char* pixels;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);//
	float* pixelsFloat = (float*)_malloca(sizeof(float) * width * height);

	//convert to single channel float texture
	//extract the alpha channel, note: ttf data is in big endian
	uint32_t pixelStride = 4;
	uint32_t counter = 0;
	for (uint32_t y = 0; y < (uint32_t)height; ++y)
	{
		for (uint32_t x = 0; x < (uint32_t)width; ++x)
		{
			uint32_t index = (y * width) + x;
			uint32_t offset = index * pixelStride;
			uint32_t currColor = *((uint32_t*)(pixels + offset));
			float aFloat = (float)((currColor & ALPHA_MASK) >> ALPHA_SHIFT);
			pixelsFloat[index] = aFloat / 255.0f;//Vector4(rFloat, gFloat, bFloat, aFloat) / 255.0f;
		}
	}

	graphics::CreateTexture2D(pixelsFloat, width, height, PUG_FORMAT_R32F, 1, g_fontTexture);

	for (uint32_t i = 0; i < MAX_GUI_MESHES; ++i)
	{//reserve "empty" buffers
		PUG_TRY(graphics::CreateVertexBuffer(nullptr, 1, 1, g_vertexBuffers[i]));
		PUG_TRY(graphics::CreateIndexBuffer(nullptr, PUG_FORMAT_R16_UINT, 1, g_indexBuffers[i]));
	}

	return PUG_RESULT_OK;
}

PUG_RESULT pug::assets::gui::DestroyGUI()
{
	//release gui meshes
	for (uint32_t i = 0; i < PUG_COUNT_OF(g_meshes); ++i)
	{
		if (g_meshes[i].m_vbh != PUG_INVALID_ID && g_meshes[i].m_ibh != PUG_INVALID_ID)
		{
			if (IsVertexBufferValid(g_meshes[i].m_vbh))
			{
				DestroyVertexBuffer(g_meshes[i].m_vbh);
			}
			if (IsIndexBufferValid(g_meshes[i].m_ibh))
			{
				DestroyIndexBuffer(g_meshes[i].m_ibh);
			}
		}
	}

	//release font texture
	DestroyTexture2D(g_fontTexture);

	return PUG_RESULT_OK;
}

PUG_RESULT pug::assets::gui::StartGUI(
	float dt,
	pug::platform::Window* window)
{
	//update imgui data
	ImGuiIO& io = ImGui::GetIO();
	io.DeltaTime = dt;
	io.MouseDown[0] = input::GetButton(BUTTON_ID_Left);
	io.MouseDown[1] = input::GetButton(BUTTON_ID_Right);
	io.MouseDown[2] = input::GetButton(BUTTON_ID_Middle);
	//TODO mousewheel
	vmath::Int2 mousePos = input::GetMousePosition();
	io.MousePos.x = (float)mousePos.x;
	io.MousePos.y = (float)mousePos.y;

	Int2 leftTop = Int2(0, 0);
	Int2 rightBottom = window->GetSize();
	io.DisplaySize = ImVec2((float)(rightBottom.x - leftTop.x), (float)(rightBottom.y - leftTop.y));

	ImGui::NewFrame();

	g_guiHovered = io.WantCaptureMouse;

	return PUG_RESULT_OK;
}

PUG_RESULT pug::assets::gui::EndGUI(
	pug::assets::graphics::Mesh*& out_meshes,
	pug::assets::graphics::Rect*& out_scissorRects,
	Texture2DHandle*& out_textures,
	uint32_t& out_meshCount)
{
	ImGui::Render();//end the imgui frame, 
	//however, since we havent supplied imgui with a render function, 
	//we will get the draw data and process it before sending it to the renderer in the desired format
	ImDrawData* drawData = ImGui::GetDrawData();
	assert(MAX_GUI_MESHES > drawData->CmdListsCount);

	uint32_t meshCounter = 0;
	for (uint32_t i = 0; i < (uint32_t)drawData->CmdListsCount; ++i)
	{//return a mesh for each command in each commandList
		//each mesh will have the same vertex buffer reference per cmdList
		//but will have a different index buffer reference
		
		const ImDrawList* cmdList = drawData->CmdLists[i];
		const ImDrawVert* vertexData = &cmdList->VtxBuffer[0];
		const uint32_t vertexStride = sizeof(cmdList->VtxBuffer[0]);
		const uint32_t vertexCount = cmdList->VtxBuffer.size();

		//update the vertex buffer
		VertexBufferHandle& vertexBuffer = g_vertexBuffers[i];
		PUG_TRY(UpdateVertexBuffer(vertexData, vertexStride, vertexCount, vertexBuffer));

		const ImDrawIdx* indexData = &cmdList->IdxBuffer[0];
		const uint32_t indexStride = sizeof(cmdList->IdxBuffer[0]);
		const uint8_t* indexDataReadPtr = (uint8_t*)indexData;

		for (int32_t j = 0; j < cmdList->CmdBuffer.size(); ++j)
		{//write a mesh for each command
			const ImDrawCmd* cmd = &cmdList->CmdBuffer[j];
			const uint32_t indexCount = cmd->ElemCount;

			IndexBufferHandle& indexBuffer = g_indexBuffers[meshCounter];
			PUG_TRY(UpdateIndexBuffer(indexDataReadPtr, GetIndexFormatFromSize(indexStride), indexCount, indexBuffer));
			indexDataReadPtr += (indexStride * indexCount);

			g_meshes[meshCounter].m_vbh = vertexBuffer;
			g_meshes[meshCounter].m_ibh = indexBuffer;
			g_scissorRects[meshCounter] = 
			{ 
				vmath::Int2((int32_t)cmd->ClipRect.x, (int32_t)cmd->ClipRect.y), 
				vmath::Int2((int32_t)cmd->ClipRect.z, (int32_t)cmd->ClipRect.w) 
			};
			g_fontTextures[meshCounter] = g_fontTexture;

			++meshCounter;
		}

		void* endPtr = (void*)(indexData + cmdList->IdxBuffer.size());
		assert(indexDataReadPtr == endPtr);
	}
	out_meshes = g_meshes;
	out_scissorRects = g_scissorRects;
	out_textures = g_fontTextures;
	out_meshCount = meshCounter;

	return PUG_RESULT_OK;
}

bool pug::assets::gui::GUIHovered()
{
	return g_guiHovered;
}