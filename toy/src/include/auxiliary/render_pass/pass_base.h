#pragma once

#include "../../toy_platform.h"

#include "../../platform/vulkan/toy_vulkan_pipeline.h"
#include "../../platform/vulkan/toy_vulkan_driver.h"
#include "../../platform/vulkan/toy_vulkan_asset.h"

#include "../../toy_scene.h"
#include "../../toy_asset_manager.h"


typedef struct toy_built_in_vulkan_frame_resource_t {
	toy_vulkan_buffer_stack_t uniform_stack;
	void* mapping_memory;

	VkDescriptorPool descriptor_pool;

	VkCommandPool graphic_cmd_pool;
	VkCommandPool compute_cmd_pool;
}toy_built_in_vulkan_frame_resource_t;


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


typedef struct toy_built_in_vulkan_descriptor_set_layout_t {
	toy_vulkan_descriptor_set_layout_t main_camera;

	toy_vulkan_descriptor_set_layout_t single_texture;
}toy_built_in_vulkan_descriptor_set_layout_t;

typedef struct toy_built_in_descriptor_set_single_texture_t {
	toy_vulkan_descriptor_set_data_header_t header;
	toy_asset_pool_item_ref_t image_ref;
	toy_asset_pool_item_ref_t sampler_ref;
}toy_built_in_descriptor_set_single_texture_t;


typedef struct toy_built_in_shader_module_t toy_built_in_shader_module_t;

typedef void (*toy_create_built_in_vulkan_descriptor_set_layouts_fp) (
	toy_vulkan_driver_t* vk_driver,
	toy_built_in_vulkan_descriptor_set_layout_t* built_in_layouts,
	toy_error_t* error
);

typedef void (*toy_create_built_in_vulkan_pipeline_layout_fp) (
	toy_vulkan_driver_t* vk_driver,
	toy_built_in_vulkan_descriptor_set_layout_t* built_in_layouts,
	toy_vulkan_pipeline_layout_t* output,
	toy_error_t* error
);

typedef void (*toy_create_built_in_vulkan_shader_module_fp) (
	toy_vulkan_driver_t* vk_driver,
	toy_vulkan_shader_loader_t* shader_loader,
	const struct toy_built_in_vulkan_pipeline_config_t* built_in_vk_pipeline_cfg,
	toy_vulkan_pipeline_layout_t layout,
	VkRenderPass render_pass,
	const toy_allocator_t* alc,
	toy_built_in_shader_module_t* output,
	toy_error_t* error
);

typedef void (*toy_destroy_built_in_vulkan_shader_module_fp) (
	toy_built_in_shader_module_t* shader_module,
	toy_vulkan_driver_t* vk_driver,
	const toy_allocator_t* alc
);

typedef void (*toy_prepare_built_in_vulkan_shader_module_data_fp) (
	toy_built_in_shader_module_t* shader,
	toy_vulkan_driver_t* vk_driver,
	toy_built_in_vulkan_frame_resource_t* frame_res,
	toy_scene_t* scene,
	toy_asset_manager_t* asset_mgr
);

typedef void (*toy_run_built_in_vulkan_shader_fp) (
	toy_built_in_shader_module_t* shader,
	toy_built_in_vulkan_frame_resource_t* frame_res,
	toy_scene_t* scene,
	toy_vulkan_driver_t* vk_driver,
	toy_asset_manager_t* asset_mgr,
	VkCommandBuffer draw_cmd
);

struct toy_built_in_shader_module_t {
	VkPipeline pipeline;
	toy_vulkan_pipeline_layout_t layout;
	toy_create_built_in_vulkan_descriptor_set_layouts_fp create_desc_set_layouts;
	toy_create_built_in_vulkan_pipeline_layout_fp create_layout;
	toy_create_built_in_vulkan_shader_module_fp create_module;
	toy_destroy_built_in_vulkan_shader_module_fp destroy_module;
};



typedef struct toy_built_in_render_pass_module_t toy_built_in_render_pass_module_t;

typedef void (*toy_create_built_in_vulkan_render_pass_fp) (
	toy_vulkan_driver_t* vk_driver,
	const toy_allocator_t* alc,
	toy_vulkan_shader_loader_t* shader_loader,
	toy_built_in_vulkan_descriptor_set_layout_t* desc_set_layous,
	toy_built_in_render_pass_module_t* output,
	toy_error_t* error
);

typedef void (*toy_destroy_built_in_vulkan_render_pass_fp) (
	toy_vulkan_driver_t* vk_driver,
	toy_built_in_render_pass_module_t* pass,
	const toy_allocator_t* alc
);

typedef void (*toy_prepare_render_pass_fp) (
	toy_built_in_render_pass_module_t* pass,
	toy_vulkan_driver_t* vk_driver,
	toy_built_in_vulkan_frame_resource_t* frame_res,
	toy_scene_t* scene,
	toy_asset_manager_t* asset_mgr
);

typedef void (*toy_run_render_pass_fp) (
	toy_built_in_render_pass_module_t* pass,
	toy_built_in_vulkan_frame_resource_t* frame_res,
	toy_scene_t* scene,
	toy_vulkan_driver_t* vk_driver,
	toy_asset_manager_t* asset_mgr,
	toy_built_in_vulkan_descriptor_set_layout_t* built_in_desc_set_layouts,
	VkCommandBuffer cmd,
	toy_error_t* error
);

struct toy_built_in_render_pass_module_t {
	void* context;
	VkRenderPass handle;
	toy_prepare_render_pass_fp prepare;
	toy_run_render_pass_fp run;
	toy_destroy_built_in_vulkan_render_pass_fp destroy_pass;
};


TOY_EXTERN_C_START

void toy_create_built_in_vulkan_frame_resource (
	toy_vulkan_device_t* vk_device,
	VkDeviceSize uniform_buffer_size,
	toy_vulkan_memory_allocator_p vk_allocator,
	const VkAllocationCallbacks* vk_alc_cb,
	toy_built_in_vulkan_frame_resource_t* output,
	toy_error_t* error
);

void toy_destroy_built_in_vulkan_frame_resource (
	toy_vulkan_device_t* vk_device,
	toy_vulkan_memory_allocator_p vk_allocator,
	const VkAllocationCallbacks* vk_alc_cb,
	toy_built_in_vulkan_frame_resource_t* frame_res
);

void toy_destroy_built_in_vulkan_descriptor_set_layouts (
	toy_vulkan_device_t* vk_device,
	const VkAllocationCallbacks* vk_alc_cb,
	toy_built_in_vulkan_descriptor_set_layout_t* desc_set_layouts
);

void toy_init_built_in_vulkan_graphic_pipeline_config ();
const struct toy_built_in_vulkan_pipeline_config_t* toy_get_built_in_vulkan_graphic_pipeline_config ();

TOY_EXTERN_C_END
