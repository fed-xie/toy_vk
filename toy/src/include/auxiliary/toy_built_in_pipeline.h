#pragma once

#include "../toy_platform.h"

#include "../platform/vulkan/toy_vulkan.h"

#include "render_pass/pass_base.h"
#include "render_pass/main_camera.h"


typedef struct toy_built_in_pipeline_t {
	toy_built_in_vulkan_frame_resource_t frame_res[TOY_CONCURRENT_FRAME_MAX];
	toy_built_in_vulkan_descriptor_set_layout_t built_in_desc_set_layout;
	
	struct {
		toy_built_in_render_pass_module_t main_camera;
	} render_pass;

	struct {
		VkCommandBuffer draw_cmds[1];
	} cmds[TOY_CONCURRENT_FRAME_MAX];
}toy_built_in_pipeline_t;


TOY_EXTERN_C_START

void toy_create_built_in_vulkan_pipeline (
	toy_vulkan_driver_t* vk_driver,
	toy_memory_allocator_t* alc,
	toy_built_in_pipeline_t* output,
	toy_error_t* error
);

void toy_destroy_built_in_vulkan_pipeline (
	toy_vulkan_driver_t* vk_driver,
	toy_memory_allocator_t* alc,
	toy_built_in_pipeline_t* pipeline
);

void toy_draw_scene (
	toy_vulkan_driver_t* vk_driver,
	toy_scene_t* scene,
	toy_asset_manager_t* asset_mgr,
	toy_built_in_pipeline_t* pipeline,
	toy_error_t* error
);

TOY_EXTERN_C_END
