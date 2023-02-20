#include "include/toy_memory.h"
#include "toy_assert.h"


static toy_aligned_p toy_padding_L (uintptr_t raw_ptr, size_t alignment) {
	TOY_ASSERT(alignment > 0);
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


static void* toy_unpadding_L (toy_aligned_p mem) {
	TOY_ASSERT(NULL != mem);

	uintptr_t ptr = (uintptr_t)mem;
	uint16_t padding = *((uint16_t*)(ptr - sizeof(uint16_t)));
	uintptr_t raw_ptr = ptr - padding;
	return (void*)raw_ptr;
}


void toy_init_memory_stack (
	void* memory,
	size_t memory_size,
	toy_memory_stack_t* output)
{
	TOY_ASSERT(NULL != memory && 0 < memory_size && NULL != output);
	output->next = NULL;
	output->bottom = (uintptr_t)memory;
	output->size = memory_size;
	output->left_top = output->bottom;
	output->right_top = output->bottom + output->size;
}


static toy_memory_stack_p toy_alloc_memory_stack (
	size_t memory_size,
	const toy_allocator_t* alc)
{
	TOY_ASSERT(NULL != alc);

	if (toy_unlikely(memory_size <= sizeof(toy_memory_stack_t) + sizeof(uint16_t)))
		return NULL;

	uintptr_t memory = (uintptr_t)toy_alloc(alc, memory_size);
	if (toy_unlikely(NULL == (void*)memory))
		return NULL;

	toy_memory_stack_p stack = toy_padding_L(memory, sizeof(void*));
	uintptr_t stack_memory = (uintptr_t)stack + sizeof(toy_memory_stack_t);
	size_t stack_size = memory + memory_size - stack_memory;
	toy_init_memory_stack((void*)stack_memory, stack_size, stack);
	return stack;
}


static void toy_free_memory_stack (
	toy_memory_stack_p stack,
	const toy_allocator_t* alc)
{
	TOY_ASSERT(NULL != stack && NULL != alc);
	toy_free(alc, toy_unpadding_L(stack));
}


toy_inline size_t toy_get_stack_unused_size (toy_memory_stack_t* stack)
{
	return stack->right_top - stack->left_top;
}


toy_inline bool toy_is_stack_empty (toy_memory_stack_t* stack)
{
	return toy_get_stack_unused_size(stack) == stack->size;
}


void* toy_stack_alloc_L (
	toy_memory_stack_t* stack,
	size_t size)
{
	TOY_ASSERT(NULL != stack);
	if (0 == size || size > toy_get_stack_unused_size(stack))
		return NULL;

	uintptr_t ret = stack->left_top;
	stack->left_top += size;
	return (void*)ret;
}


void toy_stack_free_L (
	toy_memory_stack_t* stack,
	void* memory)
{
	TOY_ASSERT(((uintptr_t)memory < stack->left_top) && ((uintptr_t)memory >= stack->bottom));
	stack->left_top = (uintptr_t)memory;
}


void* toy_stack_alloc_R (
	toy_memory_stack_t* stack,
	size_t size)
{
	TOY_ASSERT(NULL != stack);
	if (toy_unlikely(0 == size || size > stack->size))
		return NULL;

	// make sure stack->right_top - size - sizeof(uintptr_t) >= 0
	if (stack->right_top < size + sizeof(uintptr_t))
		return NULL;

	const size_t alignment = sizeof(uintptr_t);
	uintptr_t ret = stack->right_top - size;
	ret = ret & (~(alignment - 1));

	if (ret < sizeof(uintptr_t) || ret - sizeof(uintptr_t) < stack->left_top)
		return NULL;

	uintptr_t new_top = ret - sizeof(uintptr_t);
	uintptr_t* top_ptr = (uintptr_t*)new_top;
	*top_ptr = stack->right_top;
	stack->right_top = new_top;

	return (void*)ret;
}


void toy_stack_free_R (
	toy_memory_stack_t* stack,
	void* memory)
{
	uintptr_t ptr = (uintptr_t)memory;
	TOY_ASSERT(ptr >= (sizeof(uintptr_t) + stack->right_top));

	uintptr_t right_top = *((uintptr_t*)(ptr - sizeof(uintptr_t)));
	TOY_ASSERT((right_top >= stack->right_top) && (right_top <= stack->bottom + stack->size));
	stack->right_top = right_top;
}



static toy_inline uintptr_t toy_get_memory_pool_block_addr (
	toy_memory_pool_p pool,
	uint32_t block_index)
{
	return (pool->block_area + block_index * pool->block_size);
}

void toy_init_memory_pool (
	toy_aligned_p memory,
	size_t memory_size,
	size_t block_size,
	uint32_t block_count,
	size_t alignment,
	toy_memory_pool_t* output)
{
	TOY_ASSERT(block_count > 0 && block_count < UINT32_MAX);
	TOY_ASSERT(block_size >= sizeof(uint32_t));
	TOY_ASSERT(block_count * block_size <= memory_size);
	TOY_ASSERT(alignment == 0 || ((uintptr_t)memory % alignment) == 0);

	output->next = NULL;
	output->alignment = alignment;
	output->block_size = block_size;
	output->block_count = block_count;
	output->next_free_block_index = 0;
	output->allocated_block_count = 0;
	output->block_area = (uintptr_t)memory;
	output->size = memory_size;

	for (uint32_t i = 0; i < output->block_count; ++i) {
		uint32_t* next_index_ptr = (uint32_t*)toy_get_memory_pool_block_addr(output, i);
		*next_index_ptr = i + 1;
	}
}


static toy_memory_pool_p toy_alloc_memory_pool (
	size_t memory_size,
	size_t block_size,
	const toy_allocator_t* alc)
{
	TOY_ASSERT(NULL != alc);
	TOY_ASSERT(block_size >= sizeof(uint32_t));

	if (toy_unlikely(memory_size <= sizeof(toy_memory_pool_t) + sizeof(uint16_t)))
		return NULL;

	uintptr_t memory = (uintptr_t)toy_alloc(alc, memory_size);
	if (toy_unlikely(NULL == (void*)memory))
		return NULL;

	toy_memory_pool_p pool = toy_padding_L(memory, sizeof(void*));

	uintptr_t block_memory = (uintptr_t)pool + sizeof(toy_memory_pool_t);
	size_t alignment;
	if (block_size >= sizeof(float) * 16)
		alignment = sizeof(float) * 16;
	else if (block_size >= sizeof(float) * 4)
		alignment = sizeof(float) * 4;
	else if (block_size >= sizeof(float) * 2)
		alignment = sizeof(float) * 2;
	else
		alignment = sizeof(void*);
	size_t padding = alignment - (block_memory & (alignment - 1));
	block_memory = block_memory + padding;

	if (toy_unlikely(block_memory + block_size > memory + memory_size)) {
		toy_free(alc, (void*)memory);
		return NULL;
	}
	size_t block_memory_size = (uintptr_t)memory + memory_size - block_memory;
	uint64_t block_count = block_memory_size / block_size;
	TOY_ASSERT(block_count < UINT32_MAX);
	toy_init_memory_pool((toy_aligned_p)block_memory, block_memory_size, block_size, (uint32_t)block_count, alignment, pool);
	return pool;
}


static void toy_free_memory_pool (
	toy_memory_pool_p pool,
	const toy_allocator_t* alc)
{
	TOY_ASSERT(NULL != pool && NULL != alc);
	toy_free(alc, toy_unpadding_L(pool));
}


void* toy_pool_block_alloc (toy_memory_pool_p pool, size_t size)
{
	if (pool->next_free_block_index >= pool->block_count)
		return NULL;

	if (0 == size || size > pool->block_size)
		return NULL;
	TOY_ASSERT(size == pool->block_size);

	uint32_t* next_index_ptr = (uint32_t*)toy_get_memory_pool_block_addr(pool, pool->next_free_block_index);

	pool->next_free_block_index = *next_index_ptr;
	++(pool->allocated_block_count);

	return next_index_ptr;
}


void toy_pool_block_free (toy_memory_pool_p pool, void* block)
{
	TOY_ASSERT(pool->allocated_block_count > 0);
	uintptr_t ptr = (uintptr_t)block;
	TOY_ASSERT(ptr >= pool->block_area && ptr < (pool->block_area + pool->block_size * pool->block_count));
	TOY_ASSERT((ptr - pool->block_area) % pool->block_size == 0);

	uint32_t block_index = (uint32_t)((ptr - pool->block_area) / pool->block_size);
	uint32_t* next_index_ptr = (uint32_t*)toy_get_memory_pool_block_addr(pool, block_index);
	*next_index_ptr = pool->next_free_block_index;
	pool->next_free_block_index = block_index;
	--(pool->allocated_block_count);
}



#define TOY_MEMORY_AVAIL_TAG_BIT (UINT64_C(1) << 63)

typedef struct toy_buddy_block_header_t {
	struct toy_buddy_block_header_t* prev;
	struct toy_buddy_block_header_t* next;
	uint64_t size; // use the highest bit as tag, 0-unused, 1-used
}toy_buddy_block_header_t, * toy_buddy_block_header_p;


typedef struct toy_buddy_block_footer_t {
	uint64_t size; // use the highest bit as tag, 0-unused, 1-used
}toy_buddy_block_footer_t, * toy_buddy_block_footer_p;


void toy_init_buddy_allocator (
	toy_aligned_p memory,
	size_t memory_size,
	uint32_t order_shift,
	toy_memory_buddy_t* output)
{
	TOY_ASSERT(order_shift > 6 && order_shift < 32);
	// assert memory_size is 2^N
	TOY_ASSERT(NULL != memory && memory_size > (UINT64_C(1) << order_shift));
	TOY_ASSERT((memory_size & (memory_size - 1)) == 0);

	// assert memory is aligned as sizeof(void*)
	TOY_ASSERT(0 == ((uintptr_t)memory & (sizeof(void*) - 1)));

	output->memory = (uintptr_t)memory;
	output->size = memory_size;
	output->shift = order_shift;
	output->max_order = toy_fls(memory_size);
	for (int i = 0; i < TOY_MEMORY_BUDDY_ORDER_MAX; ++i) {
		output->heads[i] = (uintptr_t)NULL;
		if ((UINT64_C(1) << i) == memory_size) {
			toy_buddy_block_header_p header = (toy_buddy_block_header_p)memory;
			header->prev = NULL;
			header->next = NULL;
			header->size = memory_size;
			toy_buddy_block_footer_p footer = (toy_buddy_block_footer_p)((uintptr_t)memory + header->size - sizeof(toy_buddy_block_footer_t));
			footer->size = memory_size;
			output->heads[i] = (uintptr_t)header;
		}
	}
}


static int toy_break_buddy_block (
	toy_memory_buddy_t* buddy,
	int order,
	size_t dst_size)
{
	TOY_ASSERT((UINT64_C(1) << order) >= dst_size);
	while ((UINT64_C(1) << (order - 1)) > dst_size) {
		if (order - 1 < buddy->shift)
			break;

		toy_buddy_block_header_t* left = (toy_buddy_block_header_t*)buddy->heads[order];
		size_t order_size = UINT64_C(1) << (order - 1);

		toy_buddy_block_header_t* right = (toy_buddy_block_header_t*)((uintptr_t)left + order_size);
		right->size = order_size;
		left->size = order_size;

		toy_buddy_block_footer_p left_footer = (toy_buddy_block_footer_p)((uintptr_t)left + order_size - sizeof(toy_buddy_block_footer_t));
		left_footer->size = order_size;
		toy_buddy_block_footer_p right_footer = (toy_buddy_block_footer_p)((uintptr_t)right + order_size - sizeof(toy_buddy_block_footer_t));
		right_footer->size = order_size;

		buddy->heads[order] = (uintptr_t)(left->next);
		if (NULL != left->next)
			left->next->prev = NULL;
		left->next = (toy_buddy_block_header_t*)buddy->heads[order - 1];
		right->next = left;
		left->prev = right;
		buddy->heads[order - 1] = (uintptr_t)right;
		right->prev = NULL;

		--order;
	}

	return order;
}


void* toy_buddy_alloc (
	toy_memory_buddy_t* buddy,
	size_t size)
{
	size_t real_size = sizeof(toy_buddy_block_header_t) + size + sizeof(toy_buddy_block_footer_t);

	for (int i = buddy->shift; i < buddy->max_order; ++i) {
		if ((UINT64_C(1) << i) < real_size || (uintptr_t)NULL == buddy->heads[i])
			continue;

		int order = toy_break_buddy_block(buddy, i, real_size);

		toy_buddy_block_header_p header = (toy_buddy_block_header_p)buddy->heads[order];
		buddy->heads[order] = (uintptr_t)header->next;
		if (NULL != header->next)
			header->next->prev = NULL;

		toy_buddy_block_footer_p footer = (toy_buddy_block_footer_p)((uintptr_t)header + header->size - sizeof(toy_buddy_block_footer_t));
		header->size |= TOY_MEMORY_AVAIL_TAG_BIT;
		footer->size |= TOY_MEMORY_AVAIL_TAG_BIT;

		uintptr_t ret = (uintptr_t)header + sizeof(toy_buddy_block_header_t);
		return (void*)ret;
	}

	return NULL;
}


static void toy_merge_buddy_block (
	toy_memory_buddy_t* buddy,
	int order,
	toy_buddy_block_header_p header,
	toy_buddy_block_footer_p footer)
{
	TOY_ASSERT(order < TOY_MEMORY_BUDDY_ORDER_MAX);
	TOY_ASSERT(NULL != header && NULL != footer);
	TOY_ASSERT(header->size == footer->size);
	while (order < TOY_MEMORY_BUDDY_ORDER_MAX - 1) {
		uintptr_t offset = (uintptr_t)header - buddy->memory;

		size_t mask = (UINT64_C(1) << (order + 1)) - 1;
		size_t mod = offset & mask;
		TOY_ASSERT((mod & (mod - 1)) == 0); // assert mod is 2^N
		toy_buddy_block_header_p buddy_header;
		toy_buddy_block_footer_p buddy_footer;
		if (!!mod) { // buddy block is left one
			if ((uintptr_t)header <= buddy->memory)
				break;
			buddy_footer = (toy_buddy_block_footer_p)((uintptr_t)header - sizeof(toy_buddy_block_footer_t));
			if (buddy_footer->size != header->size)
				break;

			buddy_header = (toy_buddy_block_header_p)((uintptr_t)header - buddy_footer->size);
			header = buddy_header;
		}
		else { // buddy block is right one
			if (offset + header->size >= buddy->memory + buddy->size)
				break;
			buddy_header = (toy_buddy_block_header_p)((uintptr_t)header + header->size);
			if (buddy_header->size != header->size)
				break;

			buddy_footer = (toy_buddy_block_footer_p)((uintptr_t)buddy_header + buddy_header->size - sizeof(toy_buddy_block_footer_t));
			footer = buddy_footer;
		}

		TOY_ASSERT(buddy_header->size == buddy_footer->size);

		if (NULL != buddy_header->next)
			buddy_header->next->prev = buddy_header->prev;
		if (NULL != buddy_header->prev)
			buddy_header->prev->next = buddy_header->next;
		if (buddy->heads[order] == (uintptr_t)buddy_header)
			buddy->heads[order] = (uintptr_t)buddy_header->next;

		size_t order_size = UINT64_C(1) << (++order);
		header->size = order_size;
		footer->size = order_size;
	}

	header->next = (toy_buddy_block_header_p)buddy->heads[order];
	header->prev = NULL;
	if ((uintptr_t)NULL != buddy->heads[order])
		((toy_buddy_block_header_p)(buddy->heads[order]))->prev = header;
	buddy->heads[order] = (uintptr_t)header;
}


void toy_buddy_free (
	toy_memory_buddy_t* buddy,
	void* memory)
{
	uintptr_t ptr = (uintptr_t)memory;
	TOY_ASSERT((ptr < (uintptr_t)(buddy->memory) + buddy->size) && (ptr > (uintptr_t)(buddy->memory)));

	toy_buddy_block_header_p header = (toy_buddy_block_header_p)(ptr - sizeof(toy_buddy_block_header_t));

	const size_t block_size = header->size & ~TOY_MEMORY_AVAIL_TAG_BIT;
	toy_buddy_block_footer_p footer = (toy_buddy_block_footer_p)((uintptr_t)header + block_size - sizeof(toy_buddy_block_footer_t));
	TOY_ASSERT(footer->size == header->size);
	header->size = block_size;
	footer->size = block_size;

	for (int i = buddy->shift; i < buddy->max_order; ++i) {
		if ((UINT64_C(1) << i) == header->size) {
			toy_merge_buddy_block(buddy, i, header, footer);
			return;
		}
	}

	TOY_ASSERT(0);
}



typedef struct toy_memory_list_chunk_header_t {
	struct toy_memory_list_chunk_header_t* next;
	size_t size; /* chunk memory size (include chunk header) */
}toy_memory_list_chunk_header_t;


void toy_init_memory_list (
	void* memory,
	size_t memory_size,
	toy_memory_list_t* output)
{
	uintptr_t start = (uintptr_t)memory;
	size_t alignment = sizeof(toy_memory_list_chunk_header_t*);
	const size_t mask = alignment - 1;
	size_t padding = (alignment - (start & mask)) & mask; // padding % alignment == padding & mask
	start += padding;

	TOY_ASSERT(start + sizeof(toy_memory_list_chunk_header_t) < (uintptr_t)memory + memory_size);

	toy_memory_list_chunk_header_t* header = (toy_memory_list_chunk_header_t*)start;
	header->next = NULL;
	header->size = memory_size - padding;

	output->next = NULL;
	output->memory = (uintptr_t)memory;
	output->size = memory_size;
	output->free_list = (uintptr_t)header;
}


static toy_memory_list_p toy_alloc_memory_list (
	size_t memory_size,
	toy_allocator_t* alc)
{
	TOY_ASSERT(NULL != alc);

	if (toy_unlikely(memory_size <= sizeof(toy_memory_list_t) + sizeof(uint16_t)))
		return NULL;

	uintptr_t memory = (uintptr_t)toy_alloc(alc, memory_size);
	if (toy_unlikely(NULL == (void*)memory))
		return NULL;

	toy_memory_list_p list = toy_padding_L(memory, sizeof(void*));
	uintptr_t list_memory = (uintptr_t)list + sizeof(toy_memory_list_t);
	size_t list_memory_size = memory + memory_size - list_memory;
	toy_init_memory_list((void*)list_memory, list_memory_size, list);
	return list;
}


static void toy_free_memory_list (
	toy_memory_list_p list,
	toy_allocator_t* alc)
{
	TOY_ASSERT(NULL != list && NULL != alc);
	toy_free(alc, toy_unpadding_L(list));
}


toy_aligned_p toy_list_chunk_alloc (
	toy_memory_list_t* list,
	size_t size)
{
	if (toy_unlikely(size == 0 || size >= list->size - sizeof(toy_memory_list_chunk_header_t)))
		return NULL;

	toy_memory_list_chunk_header_t* prev = NULL, * curr = (toy_memory_list_chunk_header_t*)list->free_list;
	while (curr) {
		if (curr->size - sizeof(toy_memory_list_chunk_header_t) < size) {
			prev = curr;
			curr = curr->next;
			continue;
		}

		uintptr_t ret = ((uintptr_t)curr) + sizeof(toy_memory_list_chunk_header_t);

		toy_memory_list_chunk_header_t* next;
		size_t alignment = sizeof(void*);
		if ((uintptr_t)curr + curr->size > ret + size + sizeof(toy_memory_list_chunk_header_t) + alignment) {
			/* too much space, split this chunk */
			uintptr_t right = ret + size;
			size_t mask = alignment - 1;
			size_t padding = (alignment - (right & mask)) & mask;
			right += padding;

			next = (toy_memory_list_chunk_header_t*)right;
			next->size = (uintptr_t)curr + curr->size - right;
			next->next = curr->next;
			curr->size = right - (uintptr_t)curr;
		}
		else {
			/* just enough space, use whole chunk */
			next = curr->next;
		}

		if (prev)
			prev->next = next;
		else
			list->free_list = (uintptr_t)next;

		return (toy_aligned_p)ret;
	}
	return NULL;
}


void toy_list_chunk_free (toy_memory_list_p list, toy_aligned_p memory)
{
	uintptr_t ptr = (uintptr_t)memory;
	TOY_ASSERT(ptr >= (uintptr_t)(list->memory) + sizeof(toy_memory_list_chunk_header_t));
	TOY_ASSERT(ptr < (uintptr_t)(list->memory) + list->size);

	toy_memory_list_chunk_header_t* curr = (toy_memory_list_chunk_header_t*)(ptr - sizeof(toy_memory_list_chunk_header_t));
	toy_memory_list_chunk_header_t* prev = NULL, * next = (toy_memory_list_chunk_header_t*)list->free_list;

	while (next && next < curr) {
		prev = next;
		next = next->next;
	}
	if (prev)
		prev->next = curr;
	else
		list->free_list = (uintptr_t)curr;
	curr->next = next;

	if (next && ((uintptr_t)curr + curr->size) == (uintptr_t)next) {
		/* merge cur to higher chunk */
		curr->size += next->size;
		curr->next = next->next;
	}
	if (prev && ((uintptr_t)prev + prev->size) == (uintptr_t)curr) {
		/* merge cur to lower chunk */
		prev->size += curr->size;
		prev->next = curr->next;
	}
}



#include <stdlib.h>
static void* toy_std_alloc (void* ctx, size_t size_in_byte) { return malloc(size_in_byte); }
static void toy_std_free (void* ctx, void* mem) { free(mem); }

toy_allocator_t toy_std_alc () {
	toy_allocator_t std_alc;
	std_alc.ctx = NULL;
	std_alc.alloc = toy_std_alloc;
	std_alc.free = toy_std_free;
	return std_alc;
}


static void* toy_pools_alloc (toy_memory_pool_p* pools, size_t block_size)
{
	TOY_ASSERT(NULL != pools && NULL != *pools);
	toy_memory_pool_p pool = *pools;
	do {
		void* ret = toy_pool_block_alloc(pool, block_size);
		if (NULL != ret)
			return ret;
		pool = pool->next;
	} while (NULL != pool);

	// Create new pool
	toy_allocator_t std_alc;
	std_alc.ctx = NULL;
	std_alc.alloc = toy_std_alloc;
	std_alc.free = toy_std_free;
	size_t memory_size = (*pools)->size + sizeof(toy_memory_pool_t) + sizeof(void*);
	pool = toy_alloc_memory_pool(memory_size, (*pools)->block_size, &std_alc);
	if (NULL == pool)
		return NULL;

	// Add pool as last one
	while (NULL != *pools)
		pools = &((*pools)->next);
	*pools = pool;

	return toy_pool_block_alloc(pool, block_size);
}


static void toy_pools_free (toy_memory_pool_p* pools, void* mem)
{
	TOY_ASSERT(NULL != pools && NULL != *pools);
	toy_memory_pool_p pool = *pools;
	do {
		if (toy_is_memory_in_pool(mem, pool)) {
			toy_pool_block_free(pool, mem);
			return;
		}
		pool = pool->next;
	} while (NULL != pool);
	TOY_ASSERT(0);
}


static void* toy_lists_alloc (toy_memory_list_p* lists, size_t size_in_byte)
{
	TOY_ASSERT(NULL != lists && NULL != *lists);
	toy_memory_list_p list = *lists;
	do {
		void* ret = toy_list_chunk_alloc(list, size_in_byte);
		if (NULL != ret)
			return ret;
		list = list->next;
	} while (NULL != list);

	// Create new pool
	toy_allocator_t std_alc = toy_std_alc();
	size_t memory_size = (*lists)->size + sizeof(void*);
	list = toy_alloc_memory_list(memory_size, &std_alc);
	if (NULL == list)
		return NULL;

	// Add pool as last one
	while (NULL != *lists)
		lists = &((*lists)->next);
	*lists = list;

	return toy_list_chunk_alloc(list, size_in_byte);
}


static void toy_lists_free (toy_memory_list_p* lists, void* mem)
{
	TOY_ASSERT(NULL != lists && NULL != *lists);
	toy_memory_list_p list = *lists;
	do {
		if (toy_is_memory_in_list(mem, list)) {
			toy_list_chunk_free(list, mem);
			return;
		}
		list = list->next;
	} while (NULL != list);
	TOY_ASSERT(0);
}


toy_memory_allocator_t* toy_create_memory_allocator (toy_memory_config_t* config)
{
	toy_allocator_t std_alc = toy_std_alc();

	toy_memory_allocator_t* alc = toy_alloc_aligned(&std_alc, sizeof(toy_memory_allocator_t), sizeof(uintptr_t));
	if (NULL == alc)
		goto FAIL_ALC;

	alc->stack = toy_alloc_memory_stack(config->stack_size, &std_alc);
	if (NULL == alc->stack)
		goto FAIL_STACK;
	alc->stack_alc_L.ctx = alc->stack;
	alc->stack_alc_L.alloc = (toy_alloc_fp)toy_stack_alloc_L;
	alc->stack_alc_L.free = (toy_free_fp)toy_stack_free_L;
	alc->stack_alc_R.ctx = alc->stack;
	alc->stack_alc_R.alloc = (toy_alloc_fp)toy_stack_alloc_R;
	alc->stack_alc_R.free = (toy_free_fp)toy_stack_free_R;

	size_t chunk_pool_size = TOY_MEMORY_CHUNK_SIZE * config->chunk_count;
	alc->chunk_pools = toy_alloc_memory_pool(chunk_pool_size, TOY_MEMORY_CHUNK_SIZE, &std_alc);
	if (NULL == alc->chunk_pools)
		goto FAIL_CHUNK_POOL;
	alc->chunk_pool_alc.ctx = &alc->chunk_pools;
	alc->chunk_pool_alc.alloc = (toy_alloc_fp)toy_pools_alloc;
	alc->chunk_pool_alc.free = (toy_free_fp)toy_pools_free;

	toy_aligned_p buddy_memory = toy_alloc_aligned(&std_alc, config->buddy_size, sizeof(void*));
	if (NULL == buddy_memory)
		goto FAIL_BUDDY;
	toy_init_buddy_allocator(buddy_memory, config->buddy_size, 7, &alc->buddy);
	alc->buddy_alc.ctx = &alc->buddy;
	alc->buddy_alc.alloc = (toy_alloc_fp)toy_buddy_alloc;
	alc->buddy_alc.free = (toy_free_fp)toy_buddy_free;

	alc->lists = toy_alloc_memory_list(config->list_size, &std_alc);
	if (NULL == alc->lists)
		goto FAIL_LIST;
	alc->list_alc.ctx = &alc->lists;
	alc->list_alc.alloc = (toy_alloc_fp)toy_lists_alloc;
	alc->list_alc.free = (toy_free_fp)toy_lists_free;

	return alc;

FAIL_LIST:
	toy_free_aligned(&std_alc, buddy_memory);
FAIL_BUDDY:
	toy_free_memory_pool(alc->chunk_pools, &std_alc);
FAIL_CHUNK_POOL:
	toy_free_memory_stack(alc->stack, &std_alc);
FAIL_STACK:
	toy_free_aligned(&std_alc, alc);
FAIL_ALC:
	return NULL;
}


void toy_destroy_memory_allocator (toy_memory_allocator_t* alc)
{
	TOY_ASSERT(NULL != alc);

	toy_allocator_t std_alc = toy_std_alc();

	toy_memory_list_p list = alc->lists;
	while (NULL != list) {
		toy_memory_list_p next_list = (toy_memory_list_p)(list->next);
		toy_free_memory_list(list, &std_alc);
		list = next_list;
	}

	toy_free_aligned(&std_alc, (toy_aligned_p)(alc->buddy.memory));

	toy_memory_pool_p pool = alc->chunk_pools;
	while (NULL != pool) {
		toy_memory_pool_p next_pool = pool->next;
		toy_free_memory_pool(pool, &std_alc);
		pool = next_pool;
	}

	toy_free_memory_stack(alc->stack, &std_alc);

	toy_free_aligned(&std_alc, alc);
}


toy_memory_stack_p toy_create_memory_stack (
	size_t stack_size,
	toy_memory_allocator_t* alc, // Just for future profile
	toy_allocator_t* output_alc_L,
	toy_allocator_t* output_alc_R)
{
	toy_allocator_t std_alc = toy_std_alc();

	toy_memory_stack_p stack = toy_alloc_memory_stack(stack_size, &std_alc);
	if (NULL == stack)
		return NULL;

	if (NULL != output_alc_L) {
		output_alc_L->ctx = stack;
		output_alc_L->alloc = (toy_alloc_fp)toy_stack_alloc_L;
		output_alc_L->free = (toy_free_fp)toy_stack_free_L;
	}
	if (NULL != output_alc_R) {
		output_alc_R->ctx = stack;
		output_alc_R->alloc = (toy_alloc_fp)toy_stack_alloc_R;
		output_alc_R->free = (toy_free_fp)toy_stack_free_R;
	}
	return stack;
}


void toy_destroy_memory_stack (
	toy_memory_stack_p stack,
	toy_memory_allocator_t* alc) // Just for future profile
{
	toy_allocator_t std_alc = toy_std_alc();
	toy_free_memory_stack(stack, &std_alc);
}


void toy_create_memory_pools (
	size_t pool_size,
	size_t block_size,
	toy_memory_allocator_t* alc, // Just for future profile
	toy_memory_pool_p* output_pools,
	toy_allocator_t* output_alc)
{
	TOY_ASSERT(NULL != output_pools && NULL != output_alc);

	toy_allocator_t std_alc = toy_std_alc();

	*output_pools = toy_alloc_memory_pool(pool_size, block_size, &std_alc);
	if (NULL == *output_pools)
		return;

	output_alc->ctx = output_pools;
	output_alc->alloc = (toy_alloc_fp)toy_pools_alloc;
	output_alc->free = (toy_free_fp)toy_pools_free;
}


void toy_destroy_memory_pools (
	toy_memory_pool_p pools,
	toy_memory_allocator_t* alc) // Just for future profile
{
	toy_allocator_t std_alc = toy_std_alc();

	toy_memory_pool_p pool = pools;
	while (NULL != pool) {
		toy_memory_pool_p next_pool = pool->next;
		toy_free_memory_pool(pool, &std_alc);
		pool = next_pool;
	}
}
