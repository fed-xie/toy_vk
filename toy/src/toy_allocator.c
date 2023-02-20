#include "include/toy_allocator.h"

#include "toy_assert.h"


toy_aligned_p toy_alloc_aligned (const toy_allocator_t* alc, size_t size_in_byte, size_t alignment) {
	TOY_ASSERT(NULL != alc);

	if (0 == size_in_byte)
		return NULL;

	if (alignment < sizeof(uint16_t))
		alignment = sizeof(uint16_t);

	uintptr_t raw_ptr = (uintptr_t)(alc->alloc(alc->ctx, size_in_byte + alignment + sizeof(uint16_t)));
	if (NULL == (void*)raw_ptr)
		return NULL;

	// assert alignment is 2^N
	const size_t mask = alignment - 1;
	TOY_ASSERT((alignment & mask) == 0);

	raw_ptr += sizeof(uint16_t);
	size_t padding = alignment - (raw_ptr & mask);
	uintptr_t ret = raw_ptr + padding;
	TOY_ASSERT(padding <= UINT16_MAX);
	*((uint16_t*)(ret - sizeof(uint16_t))) = (uint16_t)padding + (uint16_t)sizeof(uint16_t);
	return (toy_aligned_p)ret;
}


void toy_free_aligned (const toy_allocator_t* alc, toy_aligned_p mem) {
	TOY_ASSERT(NULL != alc && NULL != mem);

	uintptr_t ptr = (uintptr_t)mem;
	uint16_t padding = *((uint16_t*)(ptr - sizeof(uint16_t)));
	uintptr_t raw_ptr = ptr - padding;
	alc->free(alc->ctx, (void*)raw_ptr);
}


// find last bit set
// toy_fls(1 << 10) == 11
// toy_fls(1024) == 11
// toy_fls(1) == 1
// toy_fls(0) == 0
int toy_fls (uint64_t x)
{
	if (!x)
		return 0;

	int r = 64;
	if (!(x & UINT64_C(0xffffffff00000000))) {
		x <<= 32;
		r -= 32;
	}
	if (!(x & UINT64_C(0xffff000000000000))) {
		x <<= 16;
		r -= 16;
	}
	if (!(x & UINT64_C(0xff00000000000000))) {
		x <<= 8;
		r -= 8;
	}
	if (!(x & UINT64_C(0xf000000000000000))) {
		x <<= 4;
		r -= 4;
	}
	if (!(x & UINT64_C(0xc000000000000000))) {
		x <<= 2;
		r -= 2;
	}
	if (!(x & UINT64_C(0x8000000000000000))) {
		x <<= 1;
		r -= 1;
	}
	return r;
}
