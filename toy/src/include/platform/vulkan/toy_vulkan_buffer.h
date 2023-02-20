#pragma once

#include "../../toy_platform.h"
#include "toy_vulkan_memory.h"


typedef struct toy_vulkan_buffer_t {
	VkBuffer handle;
	VkDeviceSize size;
	VkDeviceSize alignment;
	toy_vulkan_memory_binding_t binding;
}toy_vulkan_buffer_t;


typedef struct toy_vulkan_sub_buffer_t {
	VkBuffer handle;
	VkDeviceSize offset; // Absolute offset value of VkBuffer, NOT relative value
	VkDeviceSize size;
	VkDeviceSize padding;
	toy_vulkan_buffer_t* source;
}toy_vulkan_sub_buffer_t, *toy_vulkan_sub_buffer_p;


typedef struct toy_vulkan_buffer_stack_t {
	struct toy_vulkan_buffer_stack_t* next;
	toy_vulkan_buffer_t buffer;
	VkDeviceSize top;
}toy_vulkan_buffer_stack_t;


typedef struct toy_vulkan_buffer_list_chunk_t {
	struct toy_vulkan_buffer_list_chunk_t* next;
	VkDeviceSize offset;
	VkDeviceSize size;
}toy_vulkan_buffer_list_chunk_t, *toy_vulkan_buffer_list_chunk_p;

typedef struct toy_vulkan_buffer_list_pool_t {
	struct toy_vulkan_buffer_list_pool_t* next;

	toy_vulkan_buffer_t buffer;

	toy_vulkan_buffer_list_chunk_t* chunk_head;
	toy_vulkan_buffer_list_chunk_p* next_chunk;
	toy_allocator_t chunk_alc;
}toy_vulkan_buffer_list_pool_t;


TOY_EXTERN_C_START

void toy_create_vulkan_buffer (
	VkDevice dev,
	VkBufferUsageFlags buffer_usage,
	VkDeviceSize size,
	const VkPhysicalDeviceMemoryProperties* mem_props,
	toy_vulkan_binding_allocator_t* binding_alc,
	const VkAllocationCallbacks* vk_alc_cb,
	toy_vulkan_buffer_t* output,
	toy_error_t* error
);

void toy_destroy_vulkan_buffer (
	VkDevice dev,
	toy_vulkan_buffer_t* buffer,
	const VkAllocationCallbacks* vk_alc_cb
);


void toy_init_vulkan_buffer_stack (
	toy_vulkan_buffer_t* buffer,
	toy_vulkan_buffer_stack_t* output
);

toy_inline void toy_clear_vulkan_buffer_stack (toy_vulkan_buffer_stack_t* stack) {
	stack->top = 0;
}

// return offset of buffer, return VK_WHOLE_SIZE when failed
VkDeviceSize toy_vulkan_sub_buffer_alloc_L (
	toy_vulkan_buffer_stack_t* stack,
	VkDeviceSize alignment,
	VkDeviceSize size,
	toy_vulkan_sub_buffer_t* output
);

void toy_vulkan_sub_buffer_free_L (
	toy_vulkan_buffer_stack_t* stack,
	toy_vulkan_sub_buffer_t* sub_buffer
);


void toy_create_vulkan_buffer_list_pool (
	toy_vulkan_buffer_t* buffer,
	const toy_allocator_t* chunk_alc,
	toy_vulkan_buffer_list_pool_t* output,
	toy_error_t* error
);

// toy_vulkan_buffer_t should be destroyed by caller
void toy_destroy_vulkan_buffer_list_pool (
	toy_vulkan_buffer_list_pool_t* list_pool
);

// return offset of buffer, return VK_WHOLE_SIZE when failed
VkDeviceSize toy_vulkan_sub_buffer_list_alloc (
	toy_vulkan_buffer_list_pool_t* list_pool,
	VkDeviceSize size,
	VkDeviceSize alignment,
	toy_vulkan_sub_buffer_t* output
);

void toy_vulkan_sub_buffer_list_free (
	toy_vulkan_buffer_list_pool_t* list_pool,
	toy_vulkan_sub_buffer_t* sub_buffer
);

toy_inline void* toy_map_vulkan_buffer_memory (
	VkDevice dev,
	toy_vulkan_buffer_t* buffer,
	toy_error_t* error)
{
	return toy_map_vulkan_memory(
		dev,
		buffer->binding.memory,
		buffer->binding.offset,
		buffer->size,
		buffer->binding.property_flags,
		error);
}

toy_inline void toy_unmap_vulkan_buffer_memory (
	VkDevice dev,
	toy_vulkan_buffer_t* buffer,
	toy_error_t* error)
{
	toy_unmap_vulkan_memory(
		dev,
		buffer->binding.memory,
		buffer->binding.offset,
		buffer->size,
		buffer->binding.property_flags,
		error);
}

toy_inline void* toy_map_vulkan_sub_buffer_memory (
	VkDevice dev,
	toy_vulkan_sub_buffer_p sub_buffer,
	toy_error_t* error)
{
	return toy_map_vulkan_memory(
		dev,
		sub_buffer->source->binding.memory,
		sub_buffer->source->binding.offset + sub_buffer->offset,
		sub_buffer->size,
		sub_buffer->source->binding.property_flags,
		error);
}

toy_inline void toy_unmap_vulkan_sub_buffer_memory (
	VkDevice dev,
	toy_vulkan_sub_buffer_p sub_buffer,
	toy_error_t* error)
{
	toy_unmap_vulkan_memory(
		dev,
		sub_buffer->source->binding.memory,
		sub_buffer->source->binding.offset + sub_buffer->offset,
		sub_buffer->size,
		sub_buffer->source->binding.property_flags,
		error);
}

TOY_EXTERN_C_END
