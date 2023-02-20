#pragma once

#include "../../toy_platform.h"

#include "../../platform/vulkan/toy_vulkan_pipeline.h"
#include "../../platform/vulkan/toy_vulkan_driver.h"
#include "../../platform/vulkan/toy_vulkan_asset.h"

#include "../../../include/toy_scene.h"

typedef struct toy_built_in_vulkan_frame_resource_t {
	toy_vulkan_buffer_stack_t uniform_stack;
	void* mapping_memory;

	VkDescriptorPool descriptor_pool;

	VkCommandPool graphic_cmd_pool;
	VkCommandPool compute_cmd_pool;
}toy_built_in_vulkan_frame_resource_t;


TOY_EXTERN_C_START

void toy_create_built_in_vulkan_frame_resource (
	toy_vulkan_device_t* vk_device,
	VkDeviceSize uniform_buffer_size,
	toy_vulkan_memory_allocator_p vk_allocator,
	const VkAllocationCallbacks* vk_alc_cb,
	toy_built_in_vulkan_frame_resource_t* output,
	toy_error_t* error
);

void toy_destroy_built_in_vulkan_frame_resource (
	toy_vulkan_device_t* vk_device,
	toy_vulkan_memory_allocator_p vk_allocator,
	const VkAllocationCallbacks* vk_alc_cb,
	toy_built_in_vulkan_frame_resource_t* frame_res
);

TOY_EXTERN_C_END
