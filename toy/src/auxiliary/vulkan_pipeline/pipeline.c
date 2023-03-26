#include "pipeline.h"


static struct toy_built_in_vulkan_pipeline_config_t s_built_in_pipeline_cfg;

void toy_init_built_in_vulkan_graphic_pipeline_config ()
{
	s_built_in_pipeline_cfg.vertex_input.built_in_vertex.binding_desc.binding = 0;
	s_built_in_pipeline_cfg.vertex_input.built_in_vertex.binding_desc.stride = sizeof(uint32_t);
	s_built_in_pipeline_cfg.vertex_input.built_in_vertex.binding_desc.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

	s_built_in_pipeline_cfg.vertex_input.built_in_vertex.instance_attr_desc.location = 0;
	s_built_in_pipeline_cfg.vertex_input.built_in_vertex.instance_attr_desc.binding = 0;
	s_built_in_pipeline_cfg.vertex_input.built_in_vertex.instance_attr_desc.format = VK_FORMAT_R32_UINT;
	s_built_in_pipeline_cfg.vertex_input.built_in_vertex.instance_attr_desc.offset = 0;

	s_built_in_pipeline_cfg.input_assembly.triangle_list.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	s_built_in_pipeline_cfg.input_assembly.triangle_list.pNext = NULL;
	s_built_in_pipeline_cfg.input_assembly.triangle_list.flags = 0;
	s_built_in_pipeline_cfg.input_assembly.triangle_list.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	s_built_in_pipeline_cfg.input_assembly.triangle_list.primitiveRestartEnable = VK_FALSE;

	s_built_in_pipeline_cfg.rasterization.full_back_cclock.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	s_built_in_pipeline_cfg.rasterization.full_back_cclock.pNext = NULL;
	s_built_in_pipeline_cfg.rasterization.full_back_cclock.flags = 0;
	s_built_in_pipeline_cfg.rasterization.full_back_cclock.depthClampEnable = VK_FALSE;
	s_built_in_pipeline_cfg.rasterization.full_back_cclock.rasterizerDiscardEnable = VK_FALSE; // if VK_TRUE, discard all rasterizer output
	s_built_in_pipeline_cfg.rasterization.full_back_cclock.polygonMode = VK_POLYGON_MODE_FILL;
	s_built_in_pipeline_cfg.rasterization.full_back_cclock.cullMode = VK_CULL_MODE_BACK_BIT;
	s_built_in_pipeline_cfg.rasterization.full_back_cclock.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	s_built_in_pipeline_cfg.rasterization.full_back_cclock.depthBiasEnable = VK_FALSE;
	s_built_in_pipeline_cfg.rasterization.full_back_cclock.depthBiasConstantFactor = 0.0f;
	s_built_in_pipeline_cfg.rasterization.full_back_cclock.depthBiasClamp = 0.0f;
	s_built_in_pipeline_cfg.rasterization.full_back_cclock.depthBiasSlopeFactor = 0.0f;
	s_built_in_pipeline_cfg.rasterization.full_back_cclock.lineWidth = 1.0f;

	s_built_in_pipeline_cfg.multisample.no_msaa.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	s_built_in_pipeline_cfg.multisample.no_msaa.pNext = NULL;
	s_built_in_pipeline_cfg.multisample.no_msaa.flags = 0;
	s_built_in_pipeline_cfg.multisample.no_msaa.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	s_built_in_pipeline_cfg.multisample.no_msaa.sampleShadingEnable = VK_FALSE; // before enable, check VkPhysicalDeviceFeatures::sampleRateShading
	s_built_in_pipeline_cfg.multisample.no_msaa.minSampleShading = 1.0f;
	s_built_in_pipeline_cfg.multisample.no_msaa.pSampleMask = NULL;
	s_built_in_pipeline_cfg.multisample.no_msaa.alphaToCoverageEnable = VK_FALSE;
	s_built_in_pipeline_cfg.multisample.no_msaa.alphaToOneEnable = VK_FALSE;

	VkStencilOpState empty_stencil_op_state = {
		.failOp = VK_STENCIL_OP_KEEP,
		.passOp = VK_STENCIL_OP_REPLACE,
		.depthFailOp = VK_STENCIL_OP_KEEP,
		.compareOp = VK_COMPARE_OP_NEVER,
		.compareMask = 0,
		.writeMask = 0,
		.reference = 0,
	};
	s_built_in_pipeline_cfg.depth_stencil.le_no.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	s_built_in_pipeline_cfg.depth_stencil.le_no.pNext = NULL;
	s_built_in_pipeline_cfg.depth_stencil.le_no.flags = 0;
	s_built_in_pipeline_cfg.depth_stencil.le_no.depthTestEnable = VK_TRUE;
	s_built_in_pipeline_cfg.depth_stencil.le_no.depthWriteEnable = VK_TRUE;
	s_built_in_pipeline_cfg.depth_stencil.le_no.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL; // if less or equal then pass / draw
	s_built_in_pipeline_cfg.depth_stencil.le_no.depthBoundsTestEnable = VK_FALSE;
	s_built_in_pipeline_cfg.depth_stencil.le_no.stencilTestEnable = VK_FALSE;
	s_built_in_pipeline_cfg.depth_stencil.le_no.front = empty_stencil_op_state;
	s_built_in_pipeline_cfg.depth_stencil.le_no.back = empty_stencil_op_state;
	s_built_in_pipeline_cfg.depth_stencil.le_no.minDepthBounds = 0.0f;
	s_built_in_pipeline_cfg.depth_stencil.le_no.maxDepthBounds = 1.0f;

	s_built_in_pipeline_cfg.color_blend.fix_blend.attachment.blendEnable = VK_TRUE;
	s_built_in_pipeline_cfg.color_blend.fix_blend.attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	s_built_in_pipeline_cfg.color_blend.fix_blend.attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	s_built_in_pipeline_cfg.color_blend.fix_blend.attachment.colorBlendOp = VK_BLEND_OP_ADD;
	s_built_in_pipeline_cfg.color_blend.fix_blend.attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	s_built_in_pipeline_cfg.color_blend.fix_blend.attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	s_built_in_pipeline_cfg.color_blend.fix_blend.attachment.alphaBlendOp = VK_BLEND_OP_ADD;
	s_built_in_pipeline_cfg.color_blend.fix_blend.attachment.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT |
		VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT |
		VK_COLOR_COMPONENT_A_BIT;

	s_built_in_pipeline_cfg.color_blend.fix_blend.state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	s_built_in_pipeline_cfg.color_blend.fix_blend.state.pNext = NULL;
	s_built_in_pipeline_cfg.color_blend.fix_blend.state.flags = 0;
	s_built_in_pipeline_cfg.color_blend.fix_blend.state.logicOpEnable = VK_FALSE;
	s_built_in_pipeline_cfg.color_blend.fix_blend.state.logicOp = VK_LOGIC_OP_COPY;
	s_built_in_pipeline_cfg.color_blend.fix_blend.state.attachmentCount = 1;
	s_built_in_pipeline_cfg.color_blend.fix_blend.state.pAttachments = &s_built_in_pipeline_cfg.color_blend.fix_blend.attachment;
	s_built_in_pipeline_cfg.color_blend.fix_blend.state.blendConstants[0] = 0.0f;
	s_built_in_pipeline_cfg.color_blend.fix_blend.state.blendConstants[1] = 0.0f;
	s_built_in_pipeline_cfg.color_blend.fix_blend.state.blendConstants[2] = 0.0f;
	s_built_in_pipeline_cfg.color_blend.fix_blend.state.blendConstants[3] = 0.0f;
}


const struct toy_built_in_vulkan_pipeline_config_t* toy_get_built_in_vulkan_graphic_pipeline_config ()
{
	return &s_built_in_pipeline_cfg;
}


static void create_mesh (
	toy_vulkan_driver_t* vk_driver,
	toy_vulkan_shader_loader_t* shader_loader,
	const struct toy_built_in_vulkan_pipeline_config_t* built_in_vk_pipeline_cfg,
	toy_vulkan_pipeline_layout_t* layout,
	VkRenderPass render_pass,
	const toy_allocator_t* alc,
	VkPipeline* output,
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
	pipeline_ci.layout = layout->handle;
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

	*output = pipeline_handle;

	toy_ok(error);
	return;

FAIL_PIPELINE:
	vkDestroyShaderModule(dev, fragment_shader, vk_alc_cb);
FAIL_FRAGMENT_SHADER:
	vkDestroyShaderModule(dev, vertex_shader, vk_alc_cb);
FAIL_VERTEX_SHADER:
	return;
}


static void create_shadow (
	toy_vulkan_driver_t* vk_driver,
	toy_vulkan_shader_loader_t* shader_loader,
	const struct toy_built_in_vulkan_pipeline_config_t* built_in_vk_pipeline_cfg,
	toy_vulkan_pipeline_layout_t* layout,
	VkRenderPass render_pass,
	const toy_allocator_t* alc,
	VkPipeline* output,
	toy_error_t* error)
{
	VkResult vk_err;
	VkDevice dev = vk_driver->device.handle;
	const VkAllocationCallbacks* vk_alc_cb = vk_driver->vk_alc_cb_p;

	VkShaderModule vertex_shader = toy_create_vulkan_shader_module("assets/SPIR_V/shadow_glsl_vt.spv", dev, shader_loader, vk_alc_cb, error);
	if (toy_is_failed(*error))
		goto FAIL_VERTEX_SHADER;
	VkShaderModule fragment_shader = toy_create_vulkan_shader_module("assets/SPIR_V/shadow_glsl_fg.spv", dev, shader_loader, vk_alc_cb, error);
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
	pipeline_ci.layout = layout->handle;
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

	*output = pipeline_handle;

	toy_ok(error);
	return;

FAIL_PIPELINE:
	vkDestroyShaderModule(dev, fragment_shader, vk_alc_cb);
FAIL_FRAGMENT_SHADER:
	vkDestroyShaderModule(dev, vertex_shader, vk_alc_cb);
FAIL_VERTEX_SHADER:
	return;
}


void toy_create_built_in_vulkan_pipelines (
	toy_vulkan_driver_t* vk_driver,
	toy_vulkan_shader_loader_t* shader_loader,
	toy_built_in_vulkan_descriptor_set_layout_t* desc_set_layouts,
	toy_built_in_vulkan_pipeline_layouts_t* layouts,
	toy_built_in_vulkan_render_passes_t* render_passes,
	const toy_allocator_t* alc,
	toy_built_in_vulkan_pipelines_t* output,
	toy_error_t* error)
{
	memset(output, 0, sizeof(*output));

	toy_init_built_in_vulkan_graphic_pipeline_config();

	create_mesh(
		vk_driver, shader_loader, toy_get_built_in_vulkan_graphic_pipeline_config(),
		&layouts->mesh, render_passes->main_camera, alc, &output->mesh, error);
	if (toy_is_failed(*error))
		goto FAIL;

	create_shadow(
		vk_driver, shader_loader, toy_get_built_in_vulkan_graphic_pipeline_config(),
		&layouts->shadow, render_passes->shadow, alc, &output->shadow, error);
	if (toy_is_failed(*error))
		goto FAIL;

	toy_ok(error);
	return;
FAIL:
	toy_destroy_built_in_vulkan_pipelines(vk_driver, output);
}


void toy_destroy_built_in_vulkan_pipelines (
	toy_vulkan_driver_t* vk_driver,
	toy_built_in_vulkan_pipelines_t* built_in_pipelines)
{
	VkPipeline* pipelines = (VkPipeline*)built_in_pipelines;
	const int shader_count = sizeof(*built_in_pipelines) / sizeof(VkPipeline);

	VkDevice dev = vk_driver->device.handle;
	const VkAllocationCallbacks* vk_alc_cb = vk_driver->vk_alc_cb_p;

	for (int i = 0; i < shader_count; ++i) {
		if (VK_NULL_HANDLE != pipelines[i])
			vkDestroyPipeline(dev, pipelines[i], vk_alc_cb);
	}
}
