#pragma once

#include "toy_platform.h"

#include "toy_error.h"
#include "toy_allocator.h"


TOY_EXTERN_C_START

typedef struct toy_memory_stack_t {
	struct toy_memory_stack_t* next;

	uintptr_t bottom;
	size_t size;

	uintptr_t left_top;
	uintptr_t right_top;
}toy_memory_stack_t, *toy_memory_stack_p;


void toy_init_memory_stack (
	void* memory,
	size_t memory_size,
	toy_memory_stack_t* output
);

void* toy_stack_alloc_L (
	toy_memory_stack_t* stack,
	size_t size
);

void toy_stack_free_L (
	toy_memory_stack_t* stack,
	void* memory
);

void* toy_stack_alloc_R (
	toy_memory_stack_t* stack,
	size_t size
);

void toy_stack_free_R (
	toy_memory_stack_t* stack,
	void* memory
);

toy_inline void toy_clear_stack_L (toy_memory_stack_t* stack) {
	stack->left_top = stack->bottom;
}

toy_inline void toy_clear_stack_R (toy_memory_stack_t* stack) {
	stack->right_top = stack->bottom + stack->size;
}

toy_inline void toy_clear_stack (toy_memory_stack_t* stack) {
	stack->left_top = stack->bottom;
	stack->right_top = stack->bottom + stack->size;
}



typedef struct toy_memory_pool_t {
	struct toy_memory_pool_t* next;

	size_t alignment;
	size_t block_size;
	uint32_t block_count;

	uint32_t next_free_block_index;
	uint32_t allocated_block_count;

	uintptr_t block_area;
	size_t size; // size of block_area
}toy_memory_pool_t, *toy_memory_pool_p;


void toy_init_memory_pool (
	toy_aligned_p memory,
	size_t memory_size,
	size_t block_size,
	uint32_t block_count,
	size_t alignment,
	toy_memory_pool_t* output
);

toy_inline bool toy_is_memory_in_pool (void* memory, toy_memory_pool_p pool) {
	return ((uintptr_t)memory >= pool->block_area) &&
		((uintptr_t)memory < pool->block_area + pool->block_size * pool->block_count);
}

toy_inline bool toy_memory_is_pool_empty (toy_memory_pool_t* pool) {
	return 0 == pool->allocated_block_count;
}

toy_inline bool toy_memory_is_pool_full (toy_memory_pool_t* pool) {
	return pool->allocated_block_count >= pool->block_count;
}

void* toy_pool_block_alloc (toy_memory_pool_p pool, size_t size);

void toy_pool_block_free (toy_memory_pool_p pool, void* block);



#define TOY_MEMORY_BUDDY_ORDER_MAX 30
typedef struct toy_memory_buddy_t {
	uintptr_t memory;
	size_t size; // size of buddy block, this header and footer included
	int shift;
	int max_order;
	uintptr_t heads[TOY_MEMORY_BUDDY_ORDER_MAX];
}toy_memory_buddy_t, *toy_memory_buddy_p;


void toy_init_buddy_allocator (
	toy_aligned_p memory,
	size_t memory_size,
	uint32_t order_shift,
	toy_memory_buddy_t* output
);

void* toy_buddy_alloc (
	toy_memory_buddy_t* buddy,
	size_t size
);

void toy_buddy_free (
	toy_memory_buddy_t* buddy,
	void* memory
);



typedef struct toy_memory_list_t {
	struct toy_memory_list_t* next;
	uintptr_t memory;
	size_t size; // memory size
	uintptr_t free_list;
}toy_memory_list_t, *toy_memory_list_p;


void toy_init_memory_list (
	void* memory,
	size_t memory_size,
	toy_memory_list_t* output
);

toy_aligned_p toy_list_chunk_alloc (
	toy_memory_list_t* list,
	size_t size
);

void toy_list_chunk_free (
	toy_memory_list_p list,
	toy_aligned_p memory
);

toy_inline bool toy_is_memory_in_list (void* memory, toy_memory_list_p list) {
	return (uintptr_t)memory >= list->memory && ((uintptr_t)memory < list->memory + list->size);
}


toy_allocator_t toy_std_alc (); // Standard allocator using malloc() and free() in stdlib.h


typedef struct toy_memory_allocator_t {
	toy_memory_stack_p stack;
	toy_allocator_t stack_alc_L;
	toy_allocator_t stack_alc_R;

	toy_memory_list_p lists;
	toy_allocator_t list_alc;
	
	toy_memory_buddy_t buddy;
	toy_allocator_t buddy_alc;

	toy_memory_pool_p chunk_pools; // Memory chunk pools (TOY_MEMORY_CHUNK_SIZE)
	toy_allocator_t chunk_pool_alc; // Memory chunk pools (TOY_MEMORY_CHUNK_SIZE)
}toy_memory_allocator_t;


typedef struct toy_memory_config_t {
	size_t stack_size;
	size_t buddy_size;
	size_t list_size;
	size_t chunk_count;
}toy_memory_config_t;

toy_memory_allocator_t* toy_create_memory_allocator (toy_memory_config_t* config);

void toy_destroy_memory_allocator (toy_memory_allocator_t* alc);


toy_memory_stack_p toy_create_memory_stack (
	size_t stack_size,
	toy_memory_allocator_t* alc, // Just for future profile
	toy_allocator_t* output_alc_L,
	toy_allocator_t* output_alc_R
);

void toy_destroy_memory_stack (
	toy_memory_stack_p stack,
	toy_memory_allocator_t* alc // Just for future profile
);

void toy_create_memory_pools (
	size_t pool_size,
	size_t block_size,
	toy_memory_allocator_t* alc, // Just for future profile
	toy_memory_pool_p* output_pools,
	toy_allocator_t* output_alc
);

void toy_destroy_memory_pools (
	toy_memory_pool_p pools,
	toy_memory_allocator_t* alc // Just for future profile
);

TOY_EXTERN_C_END
