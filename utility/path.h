#pragma once
#include <experimental/filesystem>
#include <vector>

std::experimental::filesystem::path MakeRelativeCanonical(
	std::experimental::filesystem::path uncanonicalPath)
{
	std::experimental::filesystem::path res;

	std::vector<std::experimental::filesystem::path> components;
	std::experimental::filesystem::path::const_iterator it = uncanonicalPath.begin();
	const std::experimental::filesystem::path::const_iterator itEnd = uncanonicalPath.end();

	for (; it != itEnd; ++it)
	{
		const auto cstr = it->c_str();
		if (cstr[0] == _FS_PERIOD)
		{
			if (cstr[1] == static_cast<std::experimental::filesystem::path::value_type>(0))
				continue; // consume "."
			else if (cstr[1] == _FS_PERIOD
				&& cstr[2] == static_cast<std::experimental::filesystem::path::value_type>(0))
			{
				// If no parent is found to remove with .., ignore ..
				// (that is, C:\..\..\..\Meow canonicalizes to C:\Meow)
				if (!components.empty())
					components.pop_back();

				continue; // consume ".."
			}
		}

		components.push_back(*it);
	}

	for (const auto& _Component : components)
	res /= _Component;

	return res;
}