#pragma once

#include "result_codes.h"
#include "../logger/logger.h"

#define PUG_MULTI_LINE_MACRO_START do{		
#define PUG_MULTI_LINE_MACRO_END }while(0);

#ifdef _DEBUG
#define PUG_TRY(func)	PUG_MULTI_LINE_MACRO_START																																		\
						PUG_RESULT pug_try_res = func;																																		\
						if(pug_try_res != PUG_RESULT_OK){pug::log::Warning(#func " failed with code: %d.\n" "File: " __FILE__ "\n" "Line: %d", pug_try_res, (unsigned)(__LINE__));}			\
						PUG_MULTI_LINE_MACRO_END
#else
#define PUG_TRY(func) func;
#endif

#ifdef _DEBUG
#define PUG_ASSERT(expr, msg) (!(expr) ? pug::log::LogAssert(msg), __debugbreak(), 0 : 1)		
#else
#define PUG_ASSERT(expr, msg) (expr);
#endif

#ifdef _MSC_VER
#define PUG_ALIGN(a) __declspec(align(a))
#elif
#error("please define ALIGN for this compiler")
#endif

#define PUG_COUNT_OF(a) ((sizeof(a)) / (sizeof(a[0])))

#define PUG_ZERO_MEM(buffer) memset(buffer, 0, sizeof(buffer))

#define KB(bytes) ((bytes) * 1024)
#define MB(bytes) ((KB(bytes)) * 1024)
#define GB(bytes) ((MB(bytes)) * 1024)