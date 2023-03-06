#include "../../include/auxiliary/render_pass/pass_base.h"

#include "../../toy_assert.h"

static VkResult toy_create_vulkan_descriptor_pool (
	VkDevice dev,
	const VkAllocationCallbacks* vk_alc_cb,
	VkDescriptorPool* output)
{
	// Todo: make desc_pool_size big enough
	VkDescriptorPoolSize desc_pool_sizes[] = {
		{
			.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1000,
		}, {
			.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = 1000,
		}, {
			.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
			.descriptorCount = 1000,
		}, {
			.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,
			.descriptorCount = 1000,
		},
	};
	const uint32_t pool_size_count = sizeof(desc_pool_sizes) / sizeof(desc_pool_sizes[0]);
	uint32_t max_sets = 0;
	for (uint32_t i = 0; i < pool_size_count; ++i)
		max_sets += desc_pool_sizes[i].descriptorCount;

	VkDescriptorPoolCreateInfo desc_pool_ci;
	desc_pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	desc_pool_ci.pNext = NULL;
	desc_pool_ci.flags = 0;
	desc_pool_ci.maxSets = max_sets;
	desc_pool_ci.poolSizeCount = pool_size_count;
	desc_pool_ci.pPoolSizes = desc_pool_sizes;
	return vkCreateDescriptorPool(dev, &desc_pool_ci, vk_alc_cb, output);
}


void toy_create_built_in_vulkan_frame_resource (
	toy_vulkan_device_t* vk_device,
	VkDeviceSize uniform_buffer_size,
	toy_vulkan_memory_allocator_p vk_allocator,
	const VkAllocationCallbacks* vk_alc_cb,
	toy_built_in_vulkan_frame_resource_t* output,
	toy_error_t* error)
{
	VkDevice dev = vk_device->handle;
	VkResult vk_err;
	vk_err = toy_create_vulkan_descriptor_pool(dev, vk_alc_cb, &output->descriptor_pool);
	if (toy_unlikely(VK_SUCCESS != vk_err)) {
		toy_err_vkerr(TOY_ERROR_CREATE_OBJECT_FAILED, vk_err, "toy_create_vulkan_descriptor_pool for frame resource failed", error);
		goto FAIL_DESC_POOL;
	}

	const VkMemoryPropertyFlags property_flags[] = {
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
	};
	const uint32_t flag_count = sizeof(property_flags) / sizeof(*property_flags);
	toy_create_vulkan_buffer(
		dev,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		property_flags, flag_count, uniform_buffer_size,
		&vk_device->physical_device.memory_properties,
		&vk_allocator->vk_list_alc, vk_allocator->vk_alc_cb_p,
		&output->uniform_stack.buffer,
		error);
	if (toy_is_failed(*error))
		goto FAIL_UNIFORM_BUFFER_STACK;
	toy_init_vulkan_buffer_stack(&output->uniform_stack.buffer, &output->uniform_stack);
	output->mapping_memory = NULL;

	VkCommandPoolCreateInfo cmd_pool_ci;
	cmd_pool_ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	cmd_pool_ci.pNext = NULL;
	cmd_pool_ci.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	cmd_pool_ci.queueFamilyIndex = vk_device->device_queue_families.graphic.family;
	TOY_ASSERT(VK_QUEUE_FAMILY_IGNORED != cmd_pool_ci.queueFamilyIndex);
	vk_err = vkCreateCommandPool(dev, &cmd_pool_ci, vk_alc_cb, &output->graphic_cmd_pool);
	if (toy_unlikely(VK_SUCCESS != vk_err)) {
		toy_err_vkerr(TOY_ERROR_CREATE_OBJECT_FAILED, vk_err, "vkCreateCommandPool for frame resource graphic_cmd_pool failed", error);
		goto FAIL_GRAPHIC_CMD_POOL;
	}

	TOY_ASSERT(VK_QUEUE_FAMILY_IGNORED != cmd_pool_ci.queueFamilyIndex);
	cmd_pool_ci.queueFamilyIndex = vk_device->device_queue_families.compute.family;
	vk_err = vkCreateCommandPool(dev, &cmd_pool_ci, vk_alc_cb, &output->compute_cmd_pool);
	if (toy_unlikely(VK_SUCCESS != vk_err)) {
		toy_err_vkerr(TOY_ERROR_CREATE_OBJECT_FAILED, vk_err, "vkCreateCommandPool for frame resource compute_cmd_pool failed", error);
		goto FAIL_COMPUTE_CMD_POOL;
	}

	toy_ok(error);
	return;

FAIL_COMPUTE_CMD_POOL:
	vkDestroyCommandPool(dev, output->graphic_cmd_pool, vk_alc_cb);
FAIL_GRAPHIC_CMD_POOL:
	vkDestroyBuffer(dev, output->uniform_stack.buffer.handle, vk_alc_cb);
	toy_free_vulkan_memory_binding(&output->uniform_stack.buffer.binding);
FAIL_UNIFORM_BUFFER_STACK:
	vkDestroyDescriptorPool(dev, output->descriptor_pool, vk_alc_cb);
FAIL_DESC_POOL:
	return;
}


void toy_destroy_built_in_vulkan_frame_resource (
	toy_vulkan_device_t* vk_device,
	toy_vulkan_memory_allocator_p vk_allocator,
	const VkAllocationCallbacks* vk_alc_cb,
	toy_built_in_vulkan_frame_resource_t* frame_res)
{
	VkDevice dev = vk_device->handle;
	toy_error_t err;

	vkDestroyCommandPool(dev, frame_res->compute_cmd_pool, vk_alc_cb);
	vkDestroyCommandPool(dev, frame_res->graphic_cmd_pool, vk_alc_cb);

	if (NULL != frame_res->mapping_memory) {
		toy_unmap_vulkan_buffer_memory(dev, &frame_res->uniform_stack.buffer, &err);
		frame_res->mapping_memory = NULL;
	}
	vkDestroyBuffer(dev, frame_res->uniform_stack.buffer.handle, vk_alc_cb);
	toy_free_vulkan_memory_binding(&frame_res->uniform_stack.buffer.binding);

	vkDestroyDescriptorPool(dev, frame_res->descriptor_pool, vk_alc_cb);
}


void toy_destroy_built_in_vulkan_descriptor_set_layouts (
	toy_vulkan_device_t* vk_device,
	const VkAllocationCallbacks* vk_alc_cb,
	toy_built_in_vulkan_descriptor_set_layout_t* desc_set_layouts)
{
	toy_vulkan_descriptor_set_layout_t* layouts = (toy_vulkan_descriptor_set_layout_t*)desc_set_layouts;
	const int layout_count = sizeof(*desc_set_layouts) / sizeof(toy_vulkan_descriptor_set_layout_t);

	for (int i = 0; i < layout_count; ++i) {
		vkDestroyDescriptorSetLayout(vk_device->handle, layouts[i].handle, vk_alc_cb);
		layouts[i].handle = VK_NULL_HANDLE;
	}
}


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
