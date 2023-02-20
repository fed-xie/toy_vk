#pragma once

#include "../../toy_platform.h"
#include "toy_vulkan_buffer.h"
#include "toy_vulkan_image.h"


typedef struct toy_vulkan_mesh_primitive_t {
	toy_vulkan_sub_buffer_t vbo; // vertex buffer object
	uint32_t vertex_count;

	toy_vulkan_sub_buffer_t ibo; // index buffer object
	uint32_t index_count;
	VkIndexType index_type;
}toy_vulkan_mesh_primitive_t, *toy_vulkan_mesh_primitive_p;


typedef struct toy_vulkan_mesh_primitive_asset_pool_t {
	toy_vulkan_buffer_list_pool_t* vbo_pools;
	toy_vulkan_buffer_list_pool_t* ibo_pools;

	toy_allocator_t* list_pool_alc; // Allocator of toy_vulkan_buffer_list_pool_t
	toy_vulkan_memory_allocator_p vk_allocator;
}toy_vulkan_mesh_primitive_asset_pool_t;


TOY_EXTERN_C_START


void toy_alloc_vulkan_mesh_primitive (
	toy_vulkan_mesh_primitive_asset_pool_t* vk_pool,
	VkDeviceSize vertex_size,
	VkDeviceSize index_size,
	toy_vulkan_mesh_primitive_t* output,
	toy_error_t* error
);

void toy_free_vulkan_mesh_primitive (
	toy_vulkan_mesh_primitive_asset_pool_t* primitive_pool,
	toy_vulkan_mesh_primitive_t* primitive
);


void toy_create_vulkan_mesh_primitive_asset_pool (
	VkDeviceSize vertex_buffer_size,
	VkDeviceSize index_buffer_size,
	toy_vulkan_memory_allocator_p vk_allocator,
	toy_vulkan_mesh_primitive_asset_pool_t* output,
	toy_error_t* error
);

void toy_destroy_vulkan_mesh_primitive_asset_pool (
	toy_vulkan_mesh_primitive_asset_pool_t* vk_pool
);

TOY_EXTERN_C_END
