#pragma once

#include "../../include/toy_platform.h"

#include "../vulkan_pipeline/base.h"


TOY_EXTERN_C_START


void toy_prepare_render_pass_main_camera (
	toy_vulkan_driver_t* vk_driver,
	toy_built_in_pipeline_t* pipeline,
	toy_scene_t* scene,
	toy_asset_manager_t* asset_mgr
);

void toy_run_render_pass_main_camera (
	toy_built_in_pipeline_t* pipeline,
	toy_built_in_vulkan_frame_resource_t* frame_res,
	toy_scene_t* scene,
	toy_vulkan_driver_t* vk_driver,
	toy_asset_manager_t* asset_mgr,
	toy_built_in_vulkan_descriptor_set_layout_t* built_in_desc_set_layouts,
	VkCommandBuffer draw_cmd,
	toy_error_t* error
);

TOY_EXTERN_C_END
