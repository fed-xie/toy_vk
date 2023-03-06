#pragma once

#include "../../toy_platform.h"
#include "toy_vulkan_device.h"
#include "toy_vulkan_asset.h"
#include "../../toy_file.h"


typedef struct toy_vulkan_descriptor_set_layout_t {
	VkDescriptorSetLayout handle;
	const VkDescriptorSetLayoutBinding* bindings;
	uint32_t binding_count;
}toy_vulkan_descriptor_set_layout_t;


typedef struct toy_vulkan_pipeline_layout_t {
	VkPipelineLayout handle;
	VkDescriptorSetLayout desc_set_layouts[4]; // at least Vulkan support 4 as VkPhysicalDeviceLimits::maxBoundDescriptorSets
}toy_vulkan_pipeline_layout_t;


typedef struct toy_vulkan_shader_loader_t {
	const toy_file_interface_t* file_api;
	const toy_allocator_t* alc;
	const toy_allocator_t* tmp_alc;
}toy_vulkan_shader_loader_t;



TOY_EXTERN_C_START

VkResult toy_create_vulkan_descriptor_set_layout (
	VkDevice dev,
	const VkDescriptorSetLayoutBinding* bindings,
	uint32_t binding_count,
	const VkAllocationCallbacks* vk_alc_cb,
	toy_vulkan_descriptor_set_layout_t* output
);

VkShaderModule toy_create_vulkan_shader_module (
	const char* utf8_path,
	VkDevice dev,
	toy_vulkan_shader_loader_t* loader,
	const VkAllocationCallbacks* vk_alc_cb,
	toy_error_t* error
);

TOY_EXTERN_C_END
