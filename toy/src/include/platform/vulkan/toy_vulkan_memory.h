#pragma once

#include "../../toy_platform.h"
#include "../../toy_error.h"
#include "../../toy_memory.h"
#include "../../toy_allocator.h"

#if __cplusplus
#include <vulkan/vulkan.hpp>
#else
#include <vulkan/vulkan.h>
#endif


TOY_EXTERN_C_START

typedef struct toy_vulkan_memory_binding_t toy_vulkan_memory_binding_t;

typedef void (*toy_vulkan_free_fp)(void* source, toy_vulkan_memory_binding_t* binding);
typedef void (*toy_vulkan_alloc_fp)(
	void* context,
	uint32_t type_index,
	const VkMemoryRequirements* req,
	toy_vulkan_memory_binding_t* output,
	toy_error_t* error
);

struct toy_vulkan_memory_binding_t {
	VkDeviceMemory memory;
	VkDeviceSize offset;
	VkDeviceSize size;
	VkMemoryPropertyFlags property_flags;

	void* source;
	toy_vulkan_free_fp free;
	VkDeviceSize padding;
};

toy_inline void toy_free_vulkan_memory_binding (toy_vulkan_memory_binding_t* binding) {
	binding->free(binding->source, binding);
}

// When free binding, FIFO like stack
typedef struct toy_vulkan_binding_allocator_t {
	void* ctx;
	toy_vulkan_alloc_fp alloc;
}toy_vulkan_binding_allocator_t;



void toy_bind_vulkan_image_memory (
	VkDevice dev,
	VkImage image,
	const VkMemoryPropertyFlags* property_flags,
	uint32_t flag_count,
	const VkPhysicalDeviceMemoryProperties* mem_props,
	toy_vulkan_binding_allocator_t* binding_alc,
	toy_vulkan_memory_binding_t* output,
	toy_error_t* error
);

void toy_bind_vulkan_buffer_memory (
	VkDevice dev,
	VkBuffer buffer,
	const VkMemoryRequirements* req,
	const VkMemoryPropertyFlags* property_flags,
	uint32_t flag_count,
	const VkPhysicalDeviceMemoryProperties* mem_props,
	toy_vulkan_binding_allocator_t* binding_alc,
	toy_vulkan_memory_binding_t* output,
	toy_error_t* error
);


typedef struct toy_vulkan_memory_stack_t {
	struct toy_vulkan_memory_stack_t* next;

	VkDeviceMemory memory;
	VkDeviceSize bottom;
	VkDeviceSize size;

	uint32_t type_index;
	VkMemoryPropertyFlags property_flags;

	VkDeviceSize left_top;
	VkDeviceSize right_top;
}toy_vulkan_memory_stack_t, *toy_vulkan_memory_stack_p;


void toy_init_vulkan_memory_stack (
	VkDeviceMemory memory,
	VkDeviceSize memory_offset,
	VkDeviceSize size,
	uint32_t type_index,
	VkMemoryPropertyFlags property_flags,
	toy_vulkan_memory_stack_t* output
);

VkResult toy_alloc_vulkan_memory_stack (
	VkDevice dev,
	VkDeviceSize size,
	uint32_t type_index,
	const VkPhysicalDeviceMemoryProperties* phy_dev_mem_props,
	const VkAllocationCallbacks* vk_alc_cb,
	toy_vulkan_memory_stack_t* output
);

void toy_vulkan_stack_free_L (
	toy_vulkan_memory_stack_p stack,
	toy_vulkan_memory_binding_t* binding
);

void toy_vulkan_stack_alloc_L (
	toy_vulkan_memory_stack_p stack,
	VkDeviceSize size,
	VkDeviceSize alignment,
	toy_vulkan_memory_binding_t* output,
	toy_error_t* error
);

void toy_vulkan_stack_free_R (
	toy_vulkan_memory_stack_p stack,
	toy_vulkan_memory_binding_t* binding
);

void toy_vulkan_stack_alloc_R (
	toy_vulkan_memory_stack_p stack,
	VkDeviceSize size,
	VkDeviceSize alignment,
	toy_vulkan_memory_binding_t* output,
	toy_error_t* error
);


typedef struct toy_vulkan_memory_pool_t {
	struct toy_vulkan_memory_pool_t* next;

	VkDeviceMemory memory;
	VkDeviceSize bottom;
	VkDeviceSize size;

	uint32_t type_index;
	VkMemoryPropertyFlags property_flags;

	VkDeviceSize block_size;
	uint16_t block_count;
	uint16_t next_free_block_index;
	uint16_t free_block_count;
	uint16_t* indices_area;
}toy_vulkan_memory_pool_t, *toy_vulkan_memory_pool_p;


void toy_alloc_vulkan_memory_pool (
	VkDevice dev,
	VkDeviceSize block_size,
	uint16_t block_count,
	uint32_t type_index,
	const VkPhysicalDeviceMemoryProperties* phy_dev_mem_props,
	const toy_allocator_t* alc,
	const VkAllocationCallbacks* vk_alc_cb,
	toy_vulkan_memory_pool_p* output,
	toy_error_t* error
);

void toy_free_vulkan_memory_pool (
	VkDevice dev,
	const toy_allocator_t* alc,
	const VkAllocationCallbacks* vk_alc_cb,
	toy_vulkan_memory_pool_p pool
);

void toy_vulkan_pool_block_free (
	toy_vulkan_memory_pool_p pool,
	toy_vulkan_memory_binding_t* binding
);

void toy_vulkan_pool_block_alloc (
	toy_vulkan_memory_pool_p pool,
	VkDeviceSize size,
	VkDeviceSize alignment,
	toy_vulkan_memory_binding_t* output,
	toy_error_t* error
);


typedef struct toy_vulkan_memory_list_chunk_t {
	struct toy_vulkan_memory_list_chunk_t* next;
	VkDeviceSize offset;
	VkDeviceSize size;
}toy_vulkan_memory_list_chunk_t, *toy_vulkan_memory_list_chunk_p;


typedef struct toy_vulkan_memory_list_t {
	struct toy_vulkan_memory_list_t* next;

	VkDeviceMemory memory;
	VkDeviceSize bottom;
	VkDeviceSize size;

	uint32_t type_index;
	VkMemoryPropertyFlags property_flags;

	toy_vulkan_memory_list_chunk_t* chunk_head;
	toy_vulkan_memory_list_chunk_p* next_chunk;
	toy_allocator_t chunk_alc;
}toy_vulkan_memory_list_t, *toy_vulkan_memory_list_p;


void toy_create_vulkan_memory_list (
	VkDevice dev,
	VkDeviceSize size,
	uint32_t type_index,
	const VkPhysicalDeviceMemoryProperties* phy_dev_mem_props,
	const VkAllocationCallbacks* vk_alloc_cb,
	const toy_allocator_t* chunk_alc, // allocator for toy_vulkan_memory_list_chunk_t
	toy_vulkan_memory_list_t* output,
	toy_error_t* error
);

void toy_destroy_vulkan_memory_list (
	VkDevice dev,
	toy_vulkan_memory_list_t* list,
	const VkAllocationCallbacks* vk_alc_cb
);

void toy_vulkan_memory_chunk_free (
	toy_vulkan_memory_list_p list,
	toy_vulkan_memory_binding_t* binding
);

void toy_vulkan_memory_list_chunk_alloc (
	toy_vulkan_memory_list_p list,
	VkDeviceSize size,
	VkDeviceSize alignment,
	toy_vulkan_memory_binding_t* output,
	toy_error_t* error
);


typedef struct toy_vulkan_memory_allocator_t {
	toy_vulkan_binding_allocator_t vk_list_alc;
	toy_vulkan_binding_allocator_t vk_std_alc;

	toy_memory_pool_p chunk_pools;
	toy_allocator_t chunk_alc;
	toy_vulkan_memory_list_p vk_mem_list[VK_MAX_MEMORY_TYPES];

	VkDevice device;
	VkPhysicalDeviceMemoryProperties memory_properties;

	toy_memory_allocator_t* mem_alc;
	VkAllocationCallbacks vk_alc_cb;
	VkAllocationCallbacks* vk_alc_cb_p;
}toy_vulkan_memory_allocator_t, *toy_vulkan_memory_allocator_p;


void toy_create_vulkan_memory_allocator (
	VkDevice dev,
	VkPhysicalDevice phy_dev,
	const VkAllocationCallbacks* vk_alc_cb,
	toy_memory_allocator_t* mem_alc,
	toy_vulkan_memory_allocator_t* output,
	toy_error_t* error
);

void toy_destroy_vulkan_memory_allocator (
	toy_vulkan_memory_allocator_t* alc
);



void* toy_map_vulkan_memory (
	VkDevice dev,
	VkDeviceMemory memory,
	VkDeviceSize offset,
	VkDeviceSize size,
	VkMemoryPropertyFlags property_flags,
	toy_error_t* error
);

void toy_unmap_vulkan_memory (
	VkDevice dev,
	VkDeviceMemory memory,
	VkDeviceSize offset,
	VkDeviceSize size,
	VkMemoryPropertyFlags property_flags,
	toy_error_t* error
);

TOY_EXTERN_C_END
