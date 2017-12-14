#pragma once

namespace pug {
namespace assets {

	class SHA1Hash
	{
	public:
		SHA1Hash() = default;
		SHA1Hash(const char* string);
		SHA1Hash(const SHA1Hash& other);//copy
		SHA1Hash(SHA1Hash&& other);//move
		~SHA1Hash();

		SHA1Hash& operator= (const SHA1Hash& other);
		SHA1Hash& operator= (SHA1Hash&& other);

		friend bool operator== (const SHA1Hash& a, const SHA1Hash& b);
		friend bool operator!= (const SHA1Hash& a, const SHA1Hash& b);

	private:
		static const size_t sha1HashSize = 20;
		char m_hash[sha1HashSize];
	};

}
}