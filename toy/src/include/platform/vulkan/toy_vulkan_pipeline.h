#pragma once

#include "../../toy_platform.h"
#include "toy_vulkan_device.h"
#include "toy_vulkan_asset.h"
#include "../../toy_file.h"

struct toy_built_in_vulkan_pipeline_config_t {
	struct {
		struct {
			VkVertexInputBindingDescription binding_desc;
			VkVertexInputAttributeDescription attribute_descs[3];
		} built_in_vertex;
	} vertex_input;
	struct {
		VkPipelineInputAssemblyStateCreateInfo triangle_list;
	} input_assembly;
	struct {
		VkPipelineRasterizationStateCreateInfo full_back_cclock; // Full mode, cull back, counter clockwise
	} rasterization;
	struct {
		VkPipelineMultisampleStateCreateInfo no_msaa;
	} multisample;
	struct {
		VkPipelineDepthStencilStateCreateInfo le_no; // Draw with depth op less_equal, no stencil
	} depth_stencil;
	struct {
		struct {
			VkPipelineColorBlendAttachmentState attachment;
			VkPipelineColorBlendStateCreateInfo state;
		} fix_blend; // default
	} color_blend;
};


typedef struct toy_vulkan_graphic_pipeline_t {
	VkPipeline handle;
	VkDescriptorSetLayout desc_set_layouts[4]; // at least Vulkan support 4 as VkPhysicalDeviceLimits::maxBoundDescriptorSets
	VkPipelineLayout layout;
}toy_vulkan_graphic_pipeline_t;


typedef struct toy_vulkan_shader_loader_t {
	const toy_file_interface_t* file_api;
	const toy_allocator_t* alc;
	const toy_allocator_t* tmp_alc;
}toy_vulkan_shader_loader_t;



TOY_EXTERN_C_START

void toy_init_built_in_vulkan_graphic_pipeline_config ();
const struct toy_built_in_vulkan_pipeline_config_t* toy_get_built_in_vulkan_graphic_pipeline_config ();


VkShaderModule toy_create_vulkan_shader_module (
	const char* utf8_path,
	VkDevice dev,
	toy_vulkan_shader_loader_t* loader,
	const VkAllocationCallbacks* vk_alc_cb,
	toy_error_t* error
);

TOY_EXTERN_C_END
