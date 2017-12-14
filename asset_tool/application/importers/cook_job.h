#pragma once
#include "asset_settings.h"

#include <experimental/filesystem>

namespace pug {
namespace assets {

	class CookJob
	{
	public:
		CookJob(
			const std::experimental::filesystem::path& a_filePath,
			const AssetSettings& a_assetSettings,
			const uint32_t a_force = 0)
			: m_filePath(a_filePath)
			, m_assetSettings(a_assetSettings)
			, m_force(a_force)
		{}
		CookJob(const CookJob& other)
			: m_filePath(other.m_filePath)
			, m_assetSettings(other.m_assetSettings)
			, m_force(other.m_force)
		{}
		CookJob(CookJob&& other)
			: m_filePath(other.m_filePath)
			, m_assetSettings(other.m_assetSettings)
			, m_force(other.m_force)
		{}

		CookJob& operator=(const CookJob& other)
		{
			m_filePath = other.m_filePath;
			m_assetSettings = other.m_assetSettings;
			m_force = other.m_force;
			return *this;
		}
		CookJob& operator=(CookJob&& other)
		{
			m_filePath = other.m_filePath;
			m_assetSettings = other.m_assetSettings;
			m_force = other.m_force;
			return *this;
		}

		const std::experimental::filesystem::path& GetPath() const
		{
			return m_filePath;
		}
		const AssetSettings& GetSettings() const
		{
			return m_assetSettings;
		}
		const uint32_t IsForced() const
		{
			return m_force;
		}

	private:
		std::experimental::filesystem::path m_filePath;
		AssetSettings m_assetSettings;
		uint32_t m_force;
	};

}
}
