#pragma once
#include <experimental/filesystem>

class TextureReference
{
public:
	TextureReference(const std::experimental::filesystem::path& a_path)
		: m_filePath(a_path)
	{}
	~TextureReference()
	{}

	const std::experimental::filesystem::path& GetReferencedPath() const
	{
		return m_filePath;
	}

private:
	std::experimental::filesystem::path m_filePath;
};