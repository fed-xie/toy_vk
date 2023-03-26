#pragma once

#include "../../include/toy_platform.h"

#include "../../include/platform/vulkan/toy_vulkan_pipeline.h"
#include "../../include/platform/vulkan/toy_vulkan_driver.h"
#include "../../include/platform/vulkan/toy_vulkan_asset.h"

#include "../../include/toy_scene.h"
#include "../../include/toy_asset_manager.h"

#include "descriptor_set_layout.h"
#include "pipeline_layout.h"
#include "render_pass.h"
#include "framebuffer.h"
#include "pipeline.h"


typedef struct toy_built_in_vulkan_frame_resource_t {
	toy_vulkan_buffer_stack_t uniform_stack;
	void* mapping_memory;

	VkDescriptorPool descriptor_pool;

	VkCommandPool graphic_cmd_pool;
	VkCommandPool compute_cmd_pool;
}toy_built_in_vulkan_frame_resource_t;


typedef struct toy_built_in_pipeline_t {
	toy_built_in_vulkan_frame_resource_t frame_res[TOY_CONCURRENT_FRAME_MAX];

	toy_built_in_vulkan_descriptor_set_layout_t desc_set_layouts;
	toy_built_in_vulkan_pipeline_layouts_t pipelline_layouts;
	toy_built_in_vulkan_render_passes_t render_passes;
	toy_built_in_vulkan_render_pass_context_t pass_context;
	toy_built_in_vulkan_pipelines_t pipelines;

	struct {
		VkCommandBuffer draw_cmds[1];
	} cmds[TOY_CONCURRENT_FRAME_MAX];
}toy_built_in_pipeline_t;


typedef uint32_t (*toy_alloc_vulkan_descriptor_set_data_fp) (
	toy_asset_manager_t* asset_mgr,
	const toy_vulkan_descriptor_set_layout_t* desc_set_layout,
	toy_error_t* error
);

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
