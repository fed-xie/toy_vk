#pragma once

#include "../../toy_platform.h"

#include "pass_base.h"


typedef struct toy_vulkan_render_pass_context_main_camera_t {
	toy_vulkan_image_t depth_image;
	VkFramebuffer* frame_buffers;

	struct {
		toy_built_in_shader_module_t mesh;
	} shaders;

	// Render data
	VkDescriptorSet desc_set;
	toy_vulkan_sub_buffer_t vp_buffer;	// camera view, project matrix
	toy_vulkan_sub_buffer_t m_buffer;	// model matrix
	toy_vulkan_sub_buffer_t inst_buffer;	// instance data
}toy_vulkan_render_pass_context_main_camera_t;


TOY_EXTERN_C_START

void toy_create_built_in_vulkan_render_pass_main_camera (
	toy_vulkan_driver_t* vk_driver,
	const toy_allocator_t* alc,
	toy_vulkan_shader_loader_t* shader_loader,
	toy_built_in_vulkan_descriptor_set_layout_t* desc_set_layous,
	toy_built_in_render_pass_module_t* output,
	toy_error_t* error
);

TOY_EXTERN_C_END
