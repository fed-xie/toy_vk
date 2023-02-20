#pragma once

#include "../../toy_platform.h"

#include "pass_base.h"


typedef struct toy_vulkan_render_pass_main_camera_t {
	VkRenderPass handle;

	toy_vulkan_image_t depth_image;
	VkFramebuffer* frame_buffers;

	toy_vulkan_graphic_pipeline_t shaders[2];

	// Render data
	toy_vulkan_sub_buffer_t vp_buffer;	// camera view, project matrix
	toy_vulkan_sub_buffer_t m_buffer;	// model matrix
}toy_vulkan_render_pass_main_camera_t;


TOY_EXTERN_C_START

void toy_create_built_in_vulkan_render_pass_main_camera (
	toy_vulkan_driver_t* vk_driver,
	const toy_allocator_t* alc,
	toy_vulkan_shader_loader_t* shader_loader,
	const struct toy_built_in_vulkan_pipeline_config_t* built_in_vk_pipeline_cfg,
	toy_vulkan_render_pass_main_camera_t* output,
	toy_error_t* error
);

void toy_destroy_built_in_vulkan_render_pass_main_camera (
	toy_vulkan_driver_t* vk_driver,
	toy_vulkan_render_pass_main_camera_t* pass,
	const toy_allocator_t* alc
);

void toy_prepare_render_pass_main_camera (
	toy_built_in_vulkan_frame_resource_t* frame_res,
	toy_scene_t* scene,
	toy_vulkan_render_pass_main_camera_t* render_pass
);

void toy_draw_render_pass_main_camera (
	toy_built_in_vulkan_frame_resource_t* frame_res,
	toy_scene_t* scene,
	toy_vulkan_driver_t* vk_driver,
	toy_vulkan_render_pass_main_camera_t* render_pass,
	VkCommandBuffer draw_cmd,
	toy_error_t* error
);

TOY_EXTERN_C_END
