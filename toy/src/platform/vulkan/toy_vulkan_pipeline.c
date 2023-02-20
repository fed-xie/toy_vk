#include "../../include/platform/vulkan/toy_vulkan_pipeline.h"

#include "../../include/toy_asset.h"
#include "../../toy_assert.h"

static struct toy_built_in_vulkan_pipeline_config_t s_built_in_pipeline_cfg;

void toy_init_built_in_vulkan_graphic_pipeline_config ()
{
	s_built_in_pipeline_cfg.vertex_input.built_in_vertex.binding_desc.binding = 0;
	s_built_in_pipeline_cfg.vertex_input.built_in_vertex.binding_desc.stride = sizeof(toy_built_in_vertex_t);
	s_built_in_pipeline_cfg.vertex_input.built_in_vertex.binding_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	s_built_in_pipeline_cfg.vertex_input.built_in_vertex.attribute_descs[0].location = 0;
	s_built_in_pipeline_cfg.vertex_input.built_in_vertex.attribute_descs[0].binding = 0;
	s_built_in_pipeline_cfg.vertex_input.built_in_vertex.attribute_descs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	s_built_in_pipeline_cfg.vertex_input.built_in_vertex.attribute_descs[0].offset = offsetof(toy_built_in_vertex_t, position);
	s_built_in_pipeline_cfg.vertex_input.built_in_vertex.attribute_descs[1].location = 1;
	s_built_in_pipeline_cfg.vertex_input.built_in_vertex.attribute_descs[1].binding = 0;
	s_built_in_pipeline_cfg.vertex_input.built_in_vertex.attribute_descs[1].format = VK_FORMAT_R32G32_SFLOAT;
	s_built_in_pipeline_cfg.vertex_input.built_in_vertex.attribute_descs[1].offset = offsetof(toy_built_in_vertex_t, tex_coord);
	s_built_in_pipeline_cfg.vertex_input.built_in_vertex.attribute_descs[2].location = 2;
	s_built_in_pipeline_cfg.vertex_input.built_in_vertex.attribute_descs[2].binding = 0;
	s_built_in_pipeline_cfg.vertex_input.built_in_vertex.attribute_descs[2].format = VK_FORMAT_R32G32B32_SFLOAT;
	s_built_in_pipeline_cfg.vertex_input.built_in_vertex.attribute_descs[2].offset = offsetof(toy_built_in_vertex_t, normal);

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


VkShaderModule toy_create_vulkan_shader_module (
	const char* utf8_path,
	VkDevice dev,
	toy_vulkan_shader_loader_t* loader,
	const VkAllocationCallbacks* vk_alc_cb,
	toy_error_t* error)
{
	VkShaderModule handle;
	size_t code_size;

	toy_aligned_p file_content = toy_load_whole_file(
		utf8_path, loader->file_api, loader->alc, loader->tmp_alc, &code_size, error);
	if (toy_is_failed(*error))
		return VK_NULL_HANDLE;

	VkShaderModuleCreateInfo ci;
	ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	ci.pNext = NULL;
	ci.flags = 0;
	ci.codeSize = code_size;
	ci.pCode = (uint32_t*)file_content;
	VkResult vk_err = vkCreateShaderModule(dev, &ci, vk_alc_cb, &handle);
	toy_free_aligned(loader->alc, file_content);
	if (VK_SUCCESS != vk_err) {
		toy_err_vkerr(TOY_ERROR_CREATE_OBJECT_FAILED, vk_err, "vkCreateShaderModule failed", error);
		return VK_NULL_HANDLE;
	}

	toy_ok(error);
	return handle;
}
