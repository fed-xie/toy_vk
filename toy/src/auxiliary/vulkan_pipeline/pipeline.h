#pragma once

#include "../../include/toy_platform.h"

#include "../../include/platform/vulkan/toy_vulkan_pipeline.h"
#include "../../include/platform/vulkan/toy_vulkan_driver.h"
#include "../../include/platform/vulkan/toy_vulkan_asset.h"

#include "descriptor_set_layout.h"
#include "pipeline_layout.h"
#include "render_pass.h"


struct toy_built_in_vulkan_pipeline_config_t {
	struct {
		struct {
			VkVertexInputBindingDescription binding_desc;
			VkVertexInputAttributeDescription instance_attr_desc;
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


typedef struct toy_built_in_vulkan_pipelines_t {
	VkPipeline mesh;
	VkPipeline shadow;
}toy_built_in_vulkan_pipelines_t;


TOY_EXTERN_C_START

void toy_init_built_in_vulkan_graphic_pipeline_config ();
const struct toy_built_in_vulkan_pipeline_config_t* toy_get_built_in_vulkan_graphic_pipeline_config ();


void toy_create_built_in_vulkan_pipelines (
	toy_vulkan_driver_t* vk_driver,
	toy_vulkan_shader_loader_t* shader_loader,
	toy_built_in_vulkan_descriptor_set_layout_t* desc_set_layouts,
	toy_built_in_vulkan_pipeline_layouts_t* layouts,
	toy_built_in_vulkan_render_passes_t* render_passes,
	const toy_allocator_t* alc,
	toy_built_in_vulkan_pipelines_t* output,
	toy_error_t* error
);

void toy_destroy_built_in_vulkan_pipelines (
	toy_vulkan_driver_t* vk_driver,
	toy_built_in_vulkan_pipelines_t* built_in_pipelines
);

TOY_EXTERN_C_END
