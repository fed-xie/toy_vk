#pragma once

#include "../../toy_platform.h"
#include "../../toy_asset.h"
#include "../../toy_error.h"
#include "toy_vulkan_asset.h"
#include "toy_vulkan_device.h"


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

VkResult toy_start_vkcmd_stage_mesh_primitive (
	toy_vulkan_asset_loader_t* loader
);

void toy_submit_vkcmd_stage_mesh_primitive (
	VkDevice dev,
	toy_vulkan_asset_loader_t* loader,
	toy_error_t* error
);

VkResult toy_start_vkcmd_stage_image (
	toy_vulkan_asset_loader_t* loader
);

// vkspec.html#synchronization-pipeline-barriers
// vkspec.html#synchronization-memory-barriers
void toy_vkcmd_stage_texture_image (
	toy_vulkan_asset_loader_t* loader,
	toy_vulkan_sub_buffer_p src_buffer,
	toy_vulkan_image_p dst_image,
	uint32_t width,
	uint32_t height,
	uint32_t mipmap_level
);

void toy_submit_vkcmd_stage_image (
	VkDevice dev,
	toy_vulkan_asset_loader_t* loader,
	toy_error_t* error
);

TOY_EXTERN_C_END
