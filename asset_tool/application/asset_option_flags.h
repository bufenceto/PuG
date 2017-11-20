#pragma once

enum MESH_FLAGS
{
	//missing data
	MESH_FLAGS_CALC_TANGENTS					= 1 << 0,
	MESH_FLAGS_CALC_NORMALS						= 1 << 1,
	MESH_FLAGS_CALC_UV_COORDINATES				= 1 << 2,
	//mesh fixes
	MESH_FLAGS_MERGE_VERTICES					= 1 << 3,
	MESH_FLAGS_REMOVE_DEGENERATE_TRIANGLES		= 1 << 4,
	MESH_FLAGS_FIX_INFACING_NORMALS				= 1 << 5,
	MESH_FLAGS_TRIANGULATE						= 1 << 6,
	MESH_FLAGS_CONVERT_TO_LEFT_HANDED			= 1 << 7,
	//scene fixes
	MESH_FLAGS_IMPROVE_CACHE_LOCALITY			= 1 << 8,
	MESH_FLAGS_REMOVE_DUPLICATE_MESH_ENTRIES	= 1 << 9,
	MESH_FLAGS_OPTIMIZE_MESHES					= 1 << 10,
	MESH_FLAGS_OPTIMIZE_SCENE_GRAPH				= 1 << 11,
	//misc
	MESH_FLAGS_VALIDATE_DATA					= 1 << 12,
};

enum TEXTURE_COMPRESSION_METHODS
{
	TEXTURE_COMPRESSION_METHODS_BC1				= 1 << 0,
	TEXTURE_COMPRESSION_METHODS_BC2				= 1 << 1,
	TEXTURE_COMPRESSION_METHODS_BC3				= 1 << 2,
	TEXTURE_COMPRESSION_METHODS_BC4				= 1 << 3,
	TEXTURE_COMPRESSION_METHODS_BC5				= 1 << 4,
	TEXTURE_COMPRESSION_METHODS_BC6				= 1 << 5,
	TEXTURE_COMPRESSION_METHODS_BC7				= 1 << 6,
};

enum TEXTURE_FLAGS
{
	TEXTURE_FLAGS_COMPRESS						= 1 << 0,
	TEXTURE_FLAGS_MAKE_POWER_OF_TWO				= 1 << 1,
	TEXTURE_FLAGS_COMPRESS_WITHOUT_ALPHA		= 1 << 2,
};