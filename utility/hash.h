#pragma once

#include <iostream>
#include <string>

#define SHA1_HASH_BYTES 20

namespace vpl {
namespace utility{

	void SHA1(const std::string &string, char* out_result, const size_t& resultSize);

}
}

