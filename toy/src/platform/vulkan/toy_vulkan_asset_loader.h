#pragma once

#include "../../include/toy_platform.h"
#include "../../include/toy_asset.h"
#include "../../include/platform/vulkan/toy_vulkan_asset.h"
#include "../../include/platform/vulkan/toy_vulkan_device.h"
#include "../../include/toy_error.h"


typedef struct toy_vulkan_asset_loader_t {
	VkCommandPool transfer_pool;
	VkCommandPool graphic_pool;

	VkCommandBuffer transfer_cmd;
	VkCommandBuffer graphic_cmd;

	VkQueue transfer_queue;
	VkQueue graphic_queue;

	VkPipelineStageFlags semaphore_wait_stage;
	VkSemaphore semaphore;
	VkFence fence;

	toy_vulkan_memory_allocator_p vk_alc;
	toy_vulkan_buffer_stack_t stage_stack;
	void* mapping_memory;
}toy_vulkan_asset_loader_t;


TOY_EXTERN_C_START

void toy_create_vulkan_asset_loader (
	toy_vulkan_device_t* vk_device,
	VkDeviceSize uniform_size,
	toy_vulkan_memory_allocator_p vk_alc,
	toy_vulkan_asset_loader_t* output,
	toy_error_t* error
);

void toy_destroy_vulkan_asset_loader (
	VkDevice dev,
	toy_vulkan_asset_loader_t* loader
);

void toy_reset_vulkan_asset_loader (
	VkDevice dev,
	toy_vulkan_asset_loader_t* loader,
	toy_error_t* error
);

void toy_map_vulkan_stage_memory (
	VkDevice dev,
	toy_vulkan_asset_loader_t* loader,
	toy_error_t* error
);

void toy_unmap_vulkan_stage_memory (
	VkDevice dev,
	toy_vulkan_asset_loader_t* loader,
	toy_error_t* error
);

void toy_clear_vulkan_stage_memory (toy_vulkan_asset_loader_t* loader);

void toy_copy_data_to_vulkan_stage_memory (
	toy_stage_data_block_t* data_blocks,
	uint32_t block_count,
	toy_vulkan_asset_loader_t* loader,
	toy_vulkan_sub_buffer_t* output_buffers,
	toy_error_t* error
);

VkResult toy_start_copy_stage_mesh_primitive_data_cmd (
	toy_vulkan_asset_loader_t* loader
);

void toy_vkcmd_copy_stage_mesh_primitive_data (
	toy_vulkan_asset_loader_t* loader,
	toy_vulkan_sub_buffer_t* vbo,
	toy_vulkan_sub_buffer_t* ibo,
	toy_vulkan_mesh_primitive_t* dst
);

void toy_submit_vulkan_mesh_primitive_loading_cmd (
	VkDevice dev,
	toy_vulkan_asset_loader_t* loader,
	toy_error_t* error
);

TOY_EXTERN_C_END
