#include "sha1.h"

#include "macro.h"
#include "utility/hash.h"

#include <memory>

using namespace pug;
using namespace pug::assets;

SHA1Hash::SHA1Hash(const char* string)
{
	size_t size = sha1HashSize;
	utility::SHA1(string, m_hash, size);
	PUG_ASSERT(size == sha1HashSize, "Size invalid!");
}

SHA1Hash::SHA1Hash(const SHA1Hash& other)//copy
{
	memcpy(m_hash, other.m_hash, sha1HashSize);
}

SHA1Hash::SHA1Hash(SHA1Hash&& other)//move
{
	memcpy(m_hash, other.m_hash, sha1HashSize);
	memset(other.m_hash, 0, sha1HashSize);
}

SHA1Hash::~SHA1Hash()
{
	memset(m_hash, 0, sha1HashSize);
}

SHA1Hash& SHA1Hash::operator= (const SHA1Hash& other)
{
	memcpy(m_hash, other.m_hash, sha1HashSize);
	return *this;
}

SHA1Hash& SHA1Hash::operator= (SHA1Hash&& other)
{
	memcpy(m_hash, other.m_hash, sha1HashSize);
	memset(other.m_hash, 0, sha1HashSize);
	return *this;
}

bool pug::assets::operator== (const SHA1Hash& a, const SHA1Hash& b)
{
	return memcmp(a.m_hash, b.m_hash, SHA1Hash::sha1HashSize) == 0;
}

bool pug::assets::operator!= (const SHA1Hash& a, const SHA1Hash& b)
{
	return memcmp(a.m_hash, b.m_hash, SHA1Hash::sha1HashSize) != 0;
}