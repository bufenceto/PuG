#pragma once
#include "asset_option_flags.h"
#include "asset_types.h"

#include <memory>
#include <cstdint>

namespace pug {
namespace assets {

	struct TextureSettings
	{
		TEXTURE_COMPRESSION_METHODS m_compressionMethod;
		uint8_t m_compress;
		uint8_t m_makePowerOfTwo;
		uint8_t m_compressWithoutAlpha;
		uint32_t m_numMipsToGenerate;
	};

	struct MeshSettings
	{
		uint8_t m_calcTangents;
		uint8_t m_mergeIdenticalVertices;
		uint8_t m_generateNormals;
		uint8_t m_validateData;
		uint8_t m_improveCacheLocality;
		uint8_t m_removeDegenerateTriangles;
		uint8_t m_generateUVCoordinates;
		uint8_t m_removeDuplicateMeshEntriesFromSceneGraph;
		uint8_t m_fixInfacingNormals;
		uint8_t m_triangulate;
		uint8_t m_convertToLeftHanded;
		uint8_t m_optimizeMeshes;
		uint8_t m_optimizeSceneGraph;
		int32_t m_numLODs;
	};

	class AssetSettings
	{
	public:
		AssetSettings()
		: m_type(AssetType_Unknown)
		{
			m_meshSettings = {};
			m_texSettings = {};
		}
		AssetSettings(AssetType a_type)
			: m_type(a_type)
		{
			m_meshSettings = {};
			m_texSettings = {};
		}
		AssetSettings(const AssetSettings& other)
			: m_type(other.m_type)
			, m_texSettings(other.m_texSettings)
			, m_meshSettings(other.m_meshSettings)
		{}
		AssetSettings(AssetSettings&& other)
			: m_type(other.m_type)
			, m_texSettings(other.m_texSettings)
			, m_meshSettings(other.m_meshSettings)
		{}

		AssetSettings& operator=(const AssetSettings& other)
		{
			m_type = other.m_type;
			switch (m_type)
			{
			case AssetType_Unknown: break;
			case AssetType_Mesh: m_meshSettings = other.m_meshSettings;
			case AssetType_Texture: m_texSettings = other.m_texSettings;
			case AssetType_Material: break;
			case AssetType_Shader: break;
			}

			return *this;
		}
		AssetSettings& operator=(AssetSettings&& other)
		{
			m_type = other.m_type;
			switch (m_type)
			{
			case AssetType_Unknown: break;
			case AssetType_Mesh: m_meshSettings = other.m_meshSettings;
			case AssetType_Texture: m_texSettings = other.m_texSettings;
			case AssetType_Material: break;
			case AssetType_Shader: break;
			}

			return *this;
		}

		friend bool operator== (const AssetSettings& a, const AssetSettings& b)
		{
			if (a.m_type != b.m_type)
			{
				return false;
			}

			switch (a.m_type)
			{
			case AssetType_Unknown: return false;
			case AssetType_Mesh: return memcmp(&a.m_meshSettings, &b.m_meshSettings, sizeof(MeshSettings)) == 0;
			case AssetType_Texture : return memcmp(&a.m_texSettings, &b.m_texSettings, sizeof(TextureSettings)) == 0;
			case AssetType_Material : return false;
			case AssetType_Shader : return false;
			default:
				return false;
			}
		}
		friend bool operator!= (const AssetSettings& a, const AssetSettings& b)
		{
			return !(a == b);
		}

		AssetType m_type;
		union
		{
			TextureSettings m_texSettings;
			MeshSettings m_meshSettings;
		};
	};

}
}