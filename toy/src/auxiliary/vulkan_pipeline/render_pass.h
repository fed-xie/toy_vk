#pragma once

#include "../../include/toy_platform.h"

#include "../../include/platform/vulkan/toy_vulkan_pipeline.h"
#include "../../include/platform/vulkan/toy_vulkan_driver.h"
#include "../../include/platform/vulkan/toy_vulkan_asset.h"


typedef struct toy_built_in_vulkan_render_passes_t {
	VkRenderPass main_camera;
	VkRenderPass shadow;
}toy_built_in_vulkan_render_passes_t;


typedef struct toy_built_in_vulkan_render_pass_context_t {
	toy_vulkan_image_t camera_depth_image;
	VkFramebuffer* camera_framebuffers;

	// Render data
	VkDescriptorSet mvp_desc_set;
	toy_vulkan_sub_buffer_t vp_buffer;	// camera view, project matrix
	toy_vulkan_sub_buffer_t m_buffer;	// model matrix
	toy_vulkan_sub_buffer_t inst_buffer;	// instance data
}toy_built_in_vulkan_render_pass_context_t;


TOY_EXTERN_C_START

void toy_create_built_in_vulkan_render_passes (
	toy_vulkan_driver_t* vk_driver,
	const toy_allocator_t* alc,
	toy_built_in_vulkan_render_passes_t* output,
	toy_error_t* error
);

void toy_destroy_built_in_vulkan_render_passes (
	toy_vulkan_driver_t* vk_driver,
	const toy_allocator_t* alc,
	toy_built_in_vulkan_render_passes_t* passes
);


void toy_create_built_in_render_pass_context (
	toy_vulkan_driver_t* vk_driver,
	toy_built_in_vulkan_render_passes_t* passes,
	const toy_allocator_t* alc,
	toy_built_in_vulkan_render_pass_context_t* output,
	toy_error_t* error
);

void toy_destroy_built_in_render_pass_context (
	toy_vulkan_driver_t* vk_driver,
	toy_built_in_vulkan_render_pass_context_t* ctx,
	const toy_allocator_t* alc
);

TOY_EXTERN_C_END
