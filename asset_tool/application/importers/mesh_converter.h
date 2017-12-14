#pragma once
#include "asset_converter.h"
#include "sha1.h"

#include "macro.h"

#include <vector>
#include <Windows.h>
#include <atomic>

#define COOKED_MESH_EXTENSION ".assbin"

namespace Assimp
{
	class Importer;
	class Exporter;
}

namespace pug {
namespace assets{

	class MeshConverter : public AssetConverter
	{
	public:
		MeshConverter();
		~MeshConverter();

		bool IsExtensionSupported(
			const std::experimental::filesystem::path& a_extension) const override;
		uint32_t CookAsset(
			const std::experimental::filesystem::path& a_asset, 
			const std::experimental::filesystem::path& a_outputDirectory,
			const AssetSettings a_assetSettings) override;
		const char* GetExtension() const override { return COOKED_MESH_EXTENSION; }
		const AssetType GetAssetType() const override { return AssetType_Mesh; }

	private:
		class ManagedMutex
		{
			ManagedMutex()
			{
				InitializeCriticalSection(&m_mutex);
				m_referenceCount = 0;
			}
			~ManagedMutex()
			{
				PUG_ASSERT(m_referenceCount == 0, "This mutex has outstanding references!");
				DeleteCriticalSection(&m_mutex);
			}

			void Lock()
			{
				++m_referenceCount;
				EnterCriticalSection(&m_mutex);
			}
			void Release()
			{
				LeaveCriticalSection(&m_mutex);
				--m_referenceCount;
			}

			bool IsReferenced() const
			{
				return m_referenceCount != 0;
			}

		private:
			CRITICAL_SECTION m_mutex;
			std::atomic_uint32_t m_referenceCount;
		};

		//void WaitForActiveConversion(const SHA1Hash& a_assetHash);
		//void GuardFileForConversion(const SHA1Hash& a_assetHash);
		//
		//std::vector<std::pair<SHA1Hash, ManagedMutex*>> m_activeFileConversions;
		//CRITICAL_SECTION m_activeFileConversionListGuard;
		//
		////where our mutexes will live;
		//std::vector<ManagedMutex> m_fileMutexes;


	};

}//pug::assets
}//pug