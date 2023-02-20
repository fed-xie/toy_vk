#pragma once

#define TOY_CPP __cplusplus

#if TOY_CPP
#	define TOY_EXTERN_C_START extern "C" {
#	define TOY_EXTERN_C_END }
#	define TOY_NAME_SPACE toy
#	define TOY_NAMESPACE_START namespace TOY_NAME_SPACE {
#	define TOY_NAMESPACE_END }
#else
#	define TOY_EXTERN_C_START
#	define TOY_EXTERN_C_END
#	define TOY_NAME_SPACE
#	define TOY_NAMESPACE_START
#	define TOY_NAMESPACE_END
#endif


#define TOYAPI
#define TOYAPI_ATTR
#define TOYAPI_CALL
#define TOYAPI_PTR


#define toy_likely(x) (x)
#define toy_unlikely(x) (x)


#if defined(_MSC_VER)
#	if _MSC_VER >= 1200
#		define toy_inline __forceinline
#	endif
#else
#	define toy_inline inline
#endif


#if __cplusplus
#include <cstdalign>
#define toy_alignas(x) alignas(x)
#else
//#include <stdalign.h> // Visual Studio 2019 don't support this file yet
#define toy_alignas(x) _Alignas(x)
#endif


#define toy_bit(bit) (0x1 << (bit))
#define toy_test_bits(val, bits) (((val) & (bits)) == (bits))


#if defined(WIN32) || defined(_WIN32) || defined(WIN64) || defined(_WIN64)
#	if defined(WIN64) || defined(_WIN64)
#		define TOY_OS_WINDOWS 64
#	else
#		define TOY_OS_WINDOWS 32
#	endif
#endif

#define TOY_MEMORY_CHUNK_SIZE (32 * 1024)

#define TOY_CONCURRENT_FRAME_MAX 3

#if TOY_OS_WINDOWS
#	define TOY_DRIVER_VULKAN 1
#endif

#ifndef NDEBUG
#	define TOY_DEBUG 1
#endif

#if TOY_DEBUG
#	define TOY_DEBUG_MEMORY 1

#	if TOY_DRIVER_VULKAN
#		define TOY_DEBUG_VULKAN 1
#		if TOY_DEBUG_VULKAN
#			define TOY_DEBUG_VULKAN_BEST_PRACTICES 1
#		endif
#	endif // TOY_DRIVER_VULKAN
#endif // TOY_DEBUG
