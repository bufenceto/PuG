#pragma once
#include <cstdint>

#define PUG_RESULT uint32_t

#define PUG_RESULT_UNKNOWN 0
#define PUG_RESULT_OK 1

#define PUG_RESULT_GRAPHICS_ERROR 2
#define PUG_RESULT_PLATFORM_ERROR 3
#define PUG_RESULT_ARRAY_FULL 4

#define PUG_SUCCEEDED(result_code) (result_code == PUG_RESULT_OK)