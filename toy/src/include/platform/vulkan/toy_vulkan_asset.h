#pragma once

#include "../../toy_platform.h"
#include "toy_vulkan_buffer.h"
#include "toy_vulkan_image.h"


typedef struct toy_vulkan_mesh_primitive_t {
	uint16_t vertex_stride;
	uint16_t vertex_padding;
	uint8_t index_stride;

	uint32_t first_vertex;
	uint32_t vertex_count;

	uint32_t first_index;
	uint32_t index_count;
}toy_vulkan_mesh_primitive_t, *toy_vulkan_mesh_primitive_p;


typedef struct toy_vulkan_mesh_primitive_asset_pool_t {
	toy_vulkan_buffer_list_pool_t vbo_pool;
	toy_vulkan_buffer_list_pool_t ibo_pool8;
	toy_vulkan_buffer_list_pool_t ibo_pool16;
	toy_vulkan_buffer_list_pool_t ibo_pool32;

	toy_vulkan_memory_allocator_p vk_allocator;
}toy_vulkan_mesh_primitive_asset_pool_t;


TOY_EXTERN_C_START


void toy_create_vulkan_mesh_primitive_asset_pool (
	VkDeviceSize vertex_buffer_size,
	VkDeviceSize index_buffer8_size,
	VkDeviceSize index_buffer16_size,
	VkDeviceSize index_buffer32_size,
	toy_vulkan_memory_allocator_p vk_allocator,
	toy_vulkan_mesh_primitive_asset_pool_t* output,
	toy_error_t* error
);

void toy_destroy_vulkan_mesh_primitive_asset_pool (
	toy_vulkan_mesh_primitive_asset_pool_t* vk_pool
);

void toy_vulkan_look_up_vertex_sub_buffer (
	toy_vulkan_mesh_primitive_asset_pool_t* vk_pool,
	toy_vulkan_mesh_primitive_t* primitive,
	toy_vulkan_sub_buffer_t* output_vbo
);

void toy_vulkan_look_up_index_sub_buffer (
	toy_vulkan_mesh_primitive_asset_pool_t* vk_pool,
	toy_vulkan_mesh_primitive_t* primitive,
	toy_vulkan_sub_buffer_t* output_ibo
);

void toy_alloc_vulkan_mesh_primitive (
	toy_vulkan_mesh_primitive_asset_pool_t* vk_pool,
	uint32_t vertex_attr_size,
	uint32_t vertex_count,
	uint32_t index_count,
	toy_vulkan_mesh_primitive_t* output,
	toy_error_t* error
);

void toy_free_vulkan_mesh_primitive (
	toy_vulkan_mesh_primitive_asset_pool_t* primitive_pool,
	toy_vulkan_mesh_primitive_t* primitive
);

void toy_vkcmd_copy_mesh_primitive_data (
	VkCommandBuffer cmd,
	toy_vulkan_mesh_primitive_asset_pool_t* vk_pool,
	toy_vulkan_sub_buffer_p vbo,
	toy_vulkan_sub_buffer_p ibo,
	toy_vulkan_mesh_primitive_t* dst
);

TOY_EXTERN_C_END
