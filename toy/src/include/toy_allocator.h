#pragma once

#include "toy_platform.h"

#include "toy_error.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>

TOY_EXTERN_C_START

typedef void* (*toy_alloc_fp)(void* ctx, size_t size_in_byte);
typedef void (*toy_free_fp)(void* ctx, void* mem);
typedef struct toy_allocator_t {
	void* ctx; // context
	toy_alloc_fp alloc;
	toy_free_fp free;
}toy_allocator_t;

toy_inline void* toy_alloc (const toy_allocator_t* alc, size_t size_in_byte) {
	return alc->alloc(alc->ctx, size_in_byte);
}

toy_inline void toy_free (const toy_allocator_t* alc, void* mem) {
	alc->free(alc->ctx, mem);
}

typedef void* toy_aligned_p;
toy_aligned_p toy_alloc_aligned (const toy_allocator_t* alc, size_t size_in_byte, size_t alignment);
void toy_free_aligned (const toy_allocator_t* alc, toy_aligned_p mem);

// find last bit set
// toy_fls(1 << 10) == 11
// toy_fls(1024) == 11
// toy_fls(1) == 1
// toy_fls(0) == 0
int toy_fls (uint64_t x);

TOY_EXTERN_C_END
