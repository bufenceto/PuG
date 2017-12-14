#pragma once
#include "macro.h"
#include "texture_reference.h"

#include "vmath/vmath.h"

#include <vector>


namespace pug {
namespace assets {

	//using namespace fs = std::experimental::filesystem;

	enum TEXTURE_TYPE
	{
		TEXTURE_TYPE_DIFFUSE		= 0,
		TEXTURE_TYPE_SPECULAR		= 1,
		TEXTURE_TYPE_AMBIENT		= 2,
		TEXTURE_TYPE_EMISSIVE		= 3,
		TEXTURE_TYPE_HEIGHT			= 4,
		TEXTURE_TYPE_NORMALS		= 5,
		TEXTURE_TYPE_SHININESS		= 6,
		TEXTURE_TYPE_OPACITY		= 7,
		TEXTURE_TYPE_DISPLACEMENT	= 8,
		TEXTURE_TYPE_LIGHTMAP		= 9,
		TEXTURE_TYPE_REFLECTION		= 10,

		TEXTURE_TYPE_COUNT			= 11,

		TEXTURE_TYPE_NONE			= 12,
		TEXTURE_TYPE_UNKNOWN		= 13,
		TEXTURE_TYPE_FORCE_SIZE		= 0x7fffffff,

	};

	//aiMats can have multiple texture per type
	class Material
	{
	public:
		Material()
			: m_initialized(0)
		{

		}
		~Material()
		{

		}

		void SetDiffuseColor(const vmath::Vector3& a_diffuse)
		{
			m_diffuse = a_diffuse;
			m_initialized = 1;
		}
		void SetAmbientColor(const vmath::Vector3& a_ambient)
		{
			m_ambient = a_ambient;
			m_initialized = 1;
		}
		void SetEmissiveColor(const vmath::Vector3& a_emissive)
		{
			m_emissive = a_emissive;
			m_initialized = 1;
		}
		void SetSpecularColor(const vmath::Vector3& a_specular)
		{
			m_specular = a_specular;
			m_initialized = 1;
		}
		void SetShininess(const float a_shininess)
		{
			m_shininess = a_shininess;
			m_initialized = 1;
		}
		void SetOpacity(const float a_opacity)
		{
			m_opacity = a_opacity;
			m_initialized = 1;
		}
		void SetShininessStrength(const float a_shininessStrength)
		{
			m_shininessStrength = a_shininessStrength;
			m_initialized = 1;
		}

		void AddTexture(const TextureReference& a_texRef, const TEXTURE_TYPE a_textureType)
		{
			m_textures[a_textureType].push_back(a_texRef);
			m_initialized = 1;
		}

		uint32_t GetTexCountForType(const TEXTURE_TYPE a_textureType) const
		{
			PUG_ASSERT(a_textureType >= TEXTURE_TYPE_DIFFUSE && a_textureType <= TEXTURE_TYPE_REFLECTION, "supplied texture type is not in the correct range");

			return (uint32_t)m_textures[a_textureType].size();
		}
		const TextureReference* const GetTexturesForType(const TEXTURE_TYPE a_textureType) const
		{
			return &(m_textures[a_textureType][0]);
		}
		const vmath::Vector3& GetDiffuse() const
		{
			return m_diffuse;
		}
		const vmath::Vector3& GetAmbient() const
		{
			return m_ambient;
		}
		const vmath::Vector3& GetEmissive() const
		{
			return m_emissive;
		}
		const vmath::Vector3& GetSpecular() const
		{
			return m_specular;
		}
		const float GetShininess() const
		{
			return m_shininess;
		}
		const float GetOpacity() const
		{
			return m_opacity;
		}
		const float GetShininessStrength() const
		{
			return m_shininessStrength;
		}
		
		const uint32_t IsInitialized() const
		{
			return m_initialized;
		}

	private:
		std::vector<TextureReference> m_textures[TEXTURE_TYPE_COUNT];
		vmath::Vector3 m_diffuse;
		vmath::Vector3 m_ambient;
		vmath::Vector3 m_emissive;
		vmath::Vector3 m_specular;
		float m_shininess;
		float m_opacity;
		float m_shininessStrength;

		uint32_t m_initialized;
	};

}//pug::assets
}//pug