#pragma once

#include "../../include/toy_platform.h"

#include "../../include/platform/vulkan/toy_vulkan_pipeline.h"
#include "../../include/platform/vulkan/toy_vulkan_driver.h"
#include "../../include/platform/vulkan/toy_vulkan_asset.h"

#include "descriptor_set_layout.h"


typedef struct toy_built_in_vulkan_pipeline_layouts_t {
	toy_vulkan_pipeline_layout_t mesh;
	toy_vulkan_pipeline_layout_t shadow;
}toy_built_in_vulkan_pipeline_layouts_t;


TOY_EXTERN_C_START

void toy_create_built_in_vulkan_pipeline_layouts (
	VkDevice dev,
	const toy_built_in_vulkan_descriptor_set_layout_t* built_in_layouts,
	const VkAllocationCallbacks* vk_alc_cb,
	toy_built_in_vulkan_pipeline_layouts_t* output,
	toy_error_t* error
);

void toy_destroy_built_in_vulkan_pipeine_layouts (
	VkDevice dev,
	const VkAllocationCallbacks* vk_alc_cb,
	toy_built_in_vulkan_pipeline_layouts_t* pipeline_layouts
);

TOY_EXTERN_C_END
