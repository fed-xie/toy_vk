#pragma once

#include "../../include/toy_platform.h"

#include "../../include/platform/vulkan/toy_vulkan_pipeline.h"
#include "../../include/platform/vulkan/toy_vulkan_driver.h"
#include "../../include/platform/vulkan/toy_vulkan_asset.h"

#include "../../include/toy_asset_manager.h"


typedef void (*toy_create_built_in_vulkan_descriptor_set_layouts_fp) (
	toy_vulkan_driver_t* vk_driver,
	toy_vulkan_descriptor_set_layout_t* output,
	toy_error_t* error
);

typedef struct toy_built_in_descriptor_set_single_texture_t {
	toy_vulkan_descriptor_set_data_header_t header;
	toy_asset_pool_item_ref_t image_ref;
	toy_asset_pool_item_ref_t sampler_ref;
}toy_built_in_descriptor_set_single_texture_t;


typedef struct toy_built_in_vulkan_descriptor_set_layout_t {
	toy_vulkan_descriptor_set_layout_t main_camera;

	toy_vulkan_descriptor_set_layout_t single_texture;
}toy_built_in_vulkan_descriptor_set_layout_t;


TOY_EXTERN_C_START

uint32_t toy_alloc_vulkan_descriptor_set_single_texture (
	toy_asset_manager_t* asset_mgr,
	const toy_vulkan_descriptor_set_layout_t* desc_set_layout,
	toy_error_t* error
);

void toy_creaet_built_in_vulkan_descriptor_set_layouts (
	VkDevice dev,
	const VkAllocationCallbacks* vk_alc_cb,
	toy_built_in_vulkan_descriptor_set_layout_t* output,
	toy_error_t* error
);

void toy_destroy_built_in_vulkan_descriptor_set_layouts (
	VkDevice dev,
	const VkAllocationCallbacks* vk_alc_cb,
	toy_built_in_vulkan_descriptor_set_layout_t* desc_set_layouts
);

TOY_EXTERN_C_END
