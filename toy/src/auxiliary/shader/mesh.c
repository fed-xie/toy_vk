#include "mesh.h"

#include "../../toy_assert.h"


static VkResult create_descriptor_set_layout (
	toy_vulkan_driver_t* vk_driver,
	toy_built_in_vulkan_descriptor_set_layout_t* built_in_layouts,
	toy_error_t* error)
{
	VkDevice dev = vk_driver->device.handle;
	const VkAllocationCallbacks* vk_alc_cb = vk_driver->vk_alc_cb_p;

	static const VkDescriptorSetLayoutBinding bindings[] = {
		{
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
			.pImmutableSamplers = NULL,
		},
	};
	static const binding_count = sizeof(bindings) / sizeof(bindings[0]);
	return toy_create_vulkan_descriptor_set_layout(dev, bindings, binding_count, vk_alc_cb, &built_in_layouts->single_texture);
}


static const toy_vulkan_descriptor_set_binding_t s_desc_set_binding_single_texture[] = {
	{
		.desc_type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.binding = 0,
		.offset = offsetof(struct toy_built_in_descriptor_set_single_texture_t, image_ref),
		.size = sizeof(struct toy_built_in_descriptor_set_single_texture_t),
	},
};


uint32_t toy_alloc_vulkan_descriptor_set_single_texture (
	toy_asset_manager_t* asset_mgr,
	const toy_vulkan_descriptor_set_layout_t* desc_set_layout,
	toy_error_t* error)
{
	uint32_t material_index = toy_alloc_material(asset_mgr, sizeof(toy_built_in_descriptor_set_single_texture_t), error);
	if (toy_is_failed(*error))
		return UINT32_MAX;
	
	toy_built_in_descriptor_set_single_texture_t** desc_set_data_p = (toy_built_in_descriptor_set_single_texture_t**)toy_get_asset_item(
		&asset_mgr->asset_pools.material, material_index);
	TOY_ASSERT(NULL != desc_set_data_p);
	toy_built_in_descriptor_set_single_texture_t* desc_set_data = *desc_set_data_p;
	desc_set_data->header.desc_set_layout = desc_set_layout;
	desc_set_data->header.bindings = s_desc_set_binding_single_texture;

	toy_asset_pool_item_ref_t empty_ref;
	empty_ref.pool = NULL;
	empty_ref.index = UINT32_MAX;
	empty_ref.next_ref = UINT32_MAX;

	desc_set_data->image_ref = empty_ref;
	desc_set_data->sampler_ref = empty_ref;
	toy_ok(error);
	return material_index;
}


static void create_pipeline_layout (
	toy_vulkan_driver_t* vk_driver,
	toy_built_in_vulkan_descriptor_set_layout_t* built_in_layouts,
	toy_vulkan_pipeline_layout_t* output,
	toy_error_t* error)
{
	VkResult vk_err;
	VkDevice dev = vk_driver->device.handle;
	const VkAllocationCallbacks* vk_alc_cb = vk_driver->vk_alc_cb_p;

	VkDescriptorSetLayout desc_set_layouts[] = {
		built_in_layouts->main_camera.handle,
		built_in_layouts->single_texture.handle
	};

	VkPipelineLayout layout;
	VkPipelineLayoutCreateInfo layout_ci;
	layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layout_ci.pNext = NULL;
	layout_ci.flags = 0;
	layout_ci.setLayoutCount = sizeof(desc_set_layouts) / sizeof(*desc_set_layouts);
	layout_ci.pSetLayouts = desc_set_layouts;
	layout_ci.pushConstantRangeCount = 0;
	layout_ci.pPushConstantRanges = NULL;
	vk_err = vkCreatePipelineLayout(dev, &layout_ci, vk_alc_cb, &layout);
	if (VK_SUCCESS != vk_err) {
		toy_err_vkerr(TOY_ERROR_CREATE_OBJECT_FAILED, vk_err, "vkCreatePipelineLayout failed", error);
		return;
	}

	output->desc_set_layouts[0] = built_in_layouts->main_camera.handle;
	output->desc_set_layouts[1] = built_in_layouts->single_texture.handle;
	output->desc_set_layouts[2] = VK_NULL_HANDLE;
	output->desc_set_layouts[3] = VK_NULL_HANDLE;
	output->handle = layout;
	toy_ok(error);
}


static void create_module (
	toy_vulkan_driver_t* vk_driver,
	toy_vulkan_shader_loader_t* shader_loader,
	const struct toy_built_in_vulkan_pipeline_config_t* built_in_vk_pipeline_cfg,
	toy_vulkan_pipeline_layout_t layout,
	VkRenderPass render_pass,
	const toy_allocator_t* alc,
	toy_built_in_shader_module_t* output,
	toy_error_t* error)
{
	VkResult vk_err;
	VkDevice dev = vk_driver->device.handle;
	const VkAllocationCallbacks* vk_alc_cb = vk_driver->vk_alc_cb_p;

	VkShaderModule vertex_shader = toy_create_vulkan_shader_module("assets/SPIR_V/mesh_indirect_glsl_vt.spv", dev, shader_loader, vk_alc_cb, error);
	if (toy_is_failed(*error))
		goto FAIL_VERTEX_SHADER;
	VkShaderModule fragment_shader = toy_create_vulkan_shader_module("assets/SPIR_V/mesh_indirect_glsl_fg.spv", dev, shader_loader, vk_alc_cb, error);
	if (toy_is_failed(*error))
		goto FAIL_FRAGMENT_SHADER;

	VkPipelineShaderStageCreateInfo stages[2];
	stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[0].pNext = NULL;
	stages[0].flags = 0;
	stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	stages[0].module = vertex_shader;
	stages[0].pName = "main";
	stages[0].pSpecializationInfo = NULL;

	stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[1].pNext = NULL;
	stages[1].flags = 0;
	stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	stages[1].module = fragment_shader;
	stages[1].pName = "main";
	stages[1].pSpecializationInfo = NULL;

	VkPipelineVertexInputStateCreateInfo vertex_input;
	vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input.pNext = NULL;
	vertex_input.flags = 0;
	vertex_input.vertexBindingDescriptionCount = 0;
	vertex_input.pVertexBindingDescriptions = NULL;
	vertex_input.vertexAttributeDescriptionCount = 0;
	vertex_input.pVertexAttributeDescriptions = NULL;

	VkViewport viewport;
	viewport.x = 0;
	viewport.y = 0;
	viewport.width = (float)vk_driver->swapchain.extent.width;
	viewport.height = (float)vk_driver->swapchain.extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor;
	scissor.extent = vk_driver->swapchain.extent;
	scissor.offset.x = 0;
	scissor.offset.y = 0;

	VkPipelineViewportStateCreateInfo viewport_ci;
	viewport_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_ci.pNext = NULL;
	viewport_ci.flags = 0;
	viewport_ci.viewportCount = 1;
	viewport_ci.pViewports = &viewport;
	viewport_ci.scissorCount = 1;
	viewport_ci.pScissors = &scissor;

	VkPipeline pipeline_handle;
	VkGraphicsPipelineCreateInfo pipeline_ci;
	pipeline_ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_ci.pNext = NULL;
	pipeline_ci.flags = 0;
	pipeline_ci.stageCount = sizeof(stages) / sizeof(*stages);
	pipeline_ci.pStages = stages;
	pipeline_ci.pVertexInputState = &vertex_input;
	pipeline_ci.pInputAssemblyState = &built_in_vk_pipeline_cfg->input_assembly.triangle_list;
	pipeline_ci.pTessellationState = NULL;
	pipeline_ci.pViewportState = &viewport_ci;
	pipeline_ci.pRasterizationState = &built_in_vk_pipeline_cfg->rasterization.full_back_cclock;
	pipeline_ci.pMultisampleState = &built_in_vk_pipeline_cfg->multisample.no_msaa;
	pipeline_ci.pDepthStencilState = &built_in_vk_pipeline_cfg->depth_stencil.le_no;
	pipeline_ci.pColorBlendState = &built_in_vk_pipeline_cfg->color_blend.fix_blend.state;
	pipeline_ci.pDynamicState = NULL;
	pipeline_ci.layout = layout.handle;
	pipeline_ci.renderPass = render_pass;
	pipeline_ci.subpass = 0;
	pipeline_ci.basePipelineHandle = VK_NULL_HANDLE;
	pipeline_ci.basePipelineIndex = 0;
	vk_err = vkCreateGraphicsPipelines(
		dev,
		VK_NULL_HANDLE,
		1, &pipeline_ci,
		vk_alc_cb,
		&pipeline_handle);
	if (VK_SUCCESS != vk_err) {
		toy_err_vkerr(TOY_ERROR_CREATE_OBJECT_FAILED, vk_err, "vkCreateGraphicsPipelines failed", error);
		goto FAIL_PIPELINE;
	}

	vkDestroyShaderModule(dev, fragment_shader, vk_alc_cb);
	vkDestroyShaderModule(dev, vertex_shader, vk_alc_cb);

	output->pipeline = pipeline_handle;

	toy_ok(error);
	return;

FAIL_PIPELINE:
	vkDestroyShaderModule(dev, fragment_shader, vk_alc_cb);
FAIL_FRAGMENT_SHADER:
	vkDestroyShaderModule(dev, vertex_shader, vk_alc_cb);
FAIL_VERTEX_SHADER:
	return;
}


static void destroy_module (
	toy_built_in_shader_module_t* shader_module,
	toy_vulkan_driver_t* vk_driver,
	const toy_allocator_t* alc)
{
	vkDestroyPipeline(vk_driver->device.handle, shader_module->pipeline, vk_driver->vk_alc_cb_p);
}


toy_built_in_shader_module_t toy_get_shader_module_mesh()
{
	toy_built_in_shader_module_t shader_module;
	shader_module.pipeline = VK_NULL_HANDLE;
	shader_module.layout.handle = VK_NULL_HANDLE;
	shader_module.layout.desc_set_layouts[0] = VK_NULL_HANDLE;
	shader_module.layout.desc_set_layouts[1] = VK_NULL_HANDLE;
	shader_module.layout.desc_set_layouts[2] = VK_NULL_HANDLE;
	shader_module.layout.desc_set_layouts[3] = VK_NULL_HANDLE;
	shader_module.create_desc_set_layouts = create_descriptor_set_layout;
	shader_module.create_layout = create_pipeline_layout;
	shader_module.create_module = create_module;
	shader_module.destroy_module = destroy_module;
	return shader_module;
}
