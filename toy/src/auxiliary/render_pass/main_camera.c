#include "../../include/auxiliary/render_pass/main_camera.h"

#include "../../include/toy_file.h"
#include "../../include/toy_allocator.h"
#include "../../toy_assert.h"
#include <string.h>

static VkResult create_descriptor_set_layout_mvp (
	VkDevice dev,
	const VkAllocationCallbacks* vk_alc_cb,
	VkDescriptorSetLayout* output)
{
	VkDescriptorSetLayoutBinding bindings[] = {
		{
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.pImmutableSamplers = NULL,
		}, {
			.binding = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.pImmutableSamplers = NULL,
		},
	};

	VkDescriptorSetLayoutCreateInfo ci;
	ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	ci.pNext = NULL;
	// PUSH_DESCRIPTOR need extension VK_KHR_push_descriptor
	ci.flags = 0; // VkDescriptorSetLayoutCreateFlags: 0 | UPDATE_AFTER_BIND_POOL | PUSH_DESCRIPTOR
	ci.bindingCount = sizeof(bindings) / sizeof(bindings[0]);
	ci.pBindings = bindings;
	return vkCreateDescriptorSetLayout(dev, &ci, vk_alc_cb, output);
}


static VkResult create_descriptor_set_layout_single_texture (
	VkDevice dev,
	const VkAllocationCallbacks* vk_alc_cb,
	VkDescriptorSetLayout* output)
{
	VkDescriptorSetLayoutBinding bindings[] = {
		{
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
			.pImmutableSamplers = NULL,
		},
	};

	VkDescriptorSetLayoutCreateInfo ci;
	ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	ci.pNext = NULL;
	// PUSH_DESCRIPTOR need extension VK_KHR_push_descriptor
	ci.flags = 0; // VkDescriptorSetLayoutCreateFlags: 0 | UPDATE_AFTER_BIND_POOL | PUSH_DESCRIPTOR
	ci.bindingCount = sizeof(bindings) / sizeof(bindings[0]);
	ci.pBindings = bindings;
	return vkCreateDescriptorSetLayout(dev, &ci, vk_alc_cb, output);
}


static void create_depth_image (
	toy_vulkan_driver_t* vk_driver,
	toy_vulkan_image_t* output,
	toy_error_t* error)
{
	VkDevice dev = vk_driver->device.handle;
	const VkAllocationCallbacks* vk_alc_cb = vk_driver->vk_alc_cb_p;
	VkFormat depth_format = vk_driver->render_config.depth_format;
	VkSampleCountFlagBits msaa_count = VK_SAMPLE_COUNT_1_BIT;

	VkImageCreateInfo img_ci;
	img_ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	img_ci.pNext = NULL;
	img_ci.flags = 0;
	img_ci.imageType = VK_IMAGE_TYPE_2D;
	img_ci.format = depth_format;
	img_ci.extent.width = vk_driver->swapchain.extent.width;
	img_ci.extent.height = vk_driver->swapchain.extent.height;
	img_ci.extent.depth = 1;
	img_ci.mipLevels = 1;
	img_ci.arrayLayers = 1;
	img_ci.samples = msaa_count;
	img_ci.tiling = VK_IMAGE_TILING_OPTIMAL;
	img_ci.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	img_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	img_ci.queueFamilyIndexCount = 1;
	img_ci.pQueueFamilyIndices = NULL;
	img_ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VkResult vk_err = vkCreateImage(dev, &img_ci, vk_alc_cb, &output->handle);
	if (VK_SUCCESS != vk_err) {
		toy_err_vkerr(TOY_ERROR_CREATE_OBJECT_FAILED, vk_err, "vkCreateImage for depth image failed", error);
		return;
	}

	const VkMemoryPropertyFlags property_flags[] = { VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0 };
	const uint32_t flag_count = sizeof(property_flags) / sizeof(*property_flags);

	toy_bind_vulkan_image_memory(
		dev, output->handle,
		property_flags, flag_count,
		&vk_driver->vk_allocator.memory_properties, &vk_driver->vk_allocator.vk_list_alc,
		&output->binding,
		error);
	if (toy_is_failed(*error)) {
		vkDestroyImage(dev, output->handle, vk_alc_cb);
		return;
	}

	VkImageViewCreateInfo view_ci;
	view_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_ci.pNext = NULL;
	view_ci.flags = 0;
	view_ci.image = output->handle;
	view_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
	view_ci.format = depth_format;
	view_ci.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_ci.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_ci.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_ci.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	view_ci.subresourceRange.baseMipLevel = 0;
	view_ci.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
	view_ci.subresourceRange.baseArrayLayer = 0;
	view_ci.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
	vk_err = vkCreateImageView(dev, &view_ci, vk_alc_cb, &output->view);
	if (toy_unlikely(VK_SUCCESS != vk_err)) {
		toy_free_vulkan_memory_binding(&output->binding);
		vkDestroyImage(dev, output->handle, vk_alc_cb);
		toy_err_vkerr(TOY_ERROR_UNKNOWN_ERROR, vk_err, "vkCreateImageView for depth image failed", error);
		return;
	}

	toy_ok(error);
}


static void create_framebuffers (
	toy_vulkan_driver_t* vk_driver,
	toy_vulkan_render_pass_main_camera_t* pass,
	const toy_allocator_t* alc,
	toy_error_t* error)
{
	VkResult vk_err;

	create_depth_image(vk_driver, &pass->depth_image, error);
	if (toy_is_failed(*error))
		return;

	pass->frame_buffers = toy_alloc_aligned(alc, sizeof(VkFramebuffer) * vk_driver->swapchain.image_count, sizeof(VkFramebuffer));
	if (NULL == pass->frame_buffers) {
		toy_err(TOY_ERROR_MEMORY_HOST_ALLOCATION_FAILED, "Failed to alloc VkFramebuffer", error);
		toy_destroy_vulkan_image(vk_driver->device.handle, &pass->depth_image, vk_driver->vk_alc_cb_p);
		return;
	}

	for (uint32_t i = 0; i < vk_driver->swapchain.image_count; ++i) {
		VkImageView attachments[] = {
			vk_driver->swapchain.present_views[i],
			pass->depth_image.view,
		};
		VkFramebufferCreateInfo framebuffer_ci;
		framebuffer_ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_ci.pNext = NULL;
		framebuffer_ci.flags = 0;
		framebuffer_ci.renderPass = pass->handle;
		framebuffer_ci.attachmentCount = 2;
		framebuffer_ci.pAttachments = attachments;
		framebuffer_ci.width = vk_driver->swapchain.extent.width;
		framebuffer_ci.height = vk_driver->swapchain.extent.height;
		framebuffer_ci.layers = 1;
		vk_err = vkCreateFramebuffer(
			vk_driver->device.handle, &framebuffer_ci, vk_driver->vk_alc_cb_p, &pass->frame_buffers[i]);
		if (VK_SUCCESS != vk_err) {
			for (uint32_t j = i; j > 0; --j)
				vkDestroyFramebuffer(vk_driver->device.handle, pass->frame_buffers[j-1], vk_driver->vk_alc_cb_p);
			toy_free_aligned(alc, pass->frame_buffers);
			toy_destroy_vulkan_image(vk_driver->device.handle, &pass->depth_image, vk_driver->vk_alc_cb_p);
			toy_err_vkerr(TOY_ERROR_CREATE_OBJECT_FAILED, vk_err, "vkCreateFramebuffer failed", error);
			return;
		}
	}

	toy_ok(error);
	return;
}


static void destroy_framebuffers (
	toy_vulkan_driver_t* vk_driver,
	toy_vulkan_render_pass_main_camera_t* pass,
	const toy_allocator_t* alc)
{
	VkDevice dev = vk_driver->device.handle;
	const VkAllocationCallbacks* vk_alc_cb = vk_driver->vk_alc_cb_p;

	for (uint32_t i = vk_driver->swapchain.image_count; i > 0; --i)
		vkDestroyFramebuffer(dev, pass->frame_buffers[i - 1], vk_alc_cb);
	toy_free_aligned(alc, pass->frame_buffers);

	toy_destroy_vulkan_image(dev, &pass->depth_image, vk_alc_cb);
}


static void create_render_pass_handle (
	toy_vulkan_driver_t* vk_driver,
	VkRenderPass* output,
	toy_error_t* error)
{
	VkFormat present_format = vk_driver->device.surface.format.format;
	VkFormat depth_format = vk_driver->render_config.depth_format;
	VkSampleCountFlagBits msaa_count = VK_SAMPLE_COUNT_1_BIT;

	VkAttachmentReference refs[3];
	VkAttachmentDescription attachment_descs[3];

	// present attachment
	attachment_descs[0].flags = 0;
	attachment_descs[0].format = present_format;
	attachment_descs[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachment_descs[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachment_descs[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachment_descs[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment_descs[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachment_descs[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachment_descs[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	refs[0].attachment = 0;
	refs[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	// depth-stencil attachment
	attachment_descs[1].flags = 0;
	attachment_descs[1].format = depth_format;
	attachment_descs[1].samples = msaa_count;
	attachment_descs[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachment_descs[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // if msaa count > 1, store op must be DONT_CARE
	attachment_descs[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment_descs[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachment_descs[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachment_descs[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	refs[1].attachment = 1;
	refs[1].layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	// msaa sample attachment
	attachment_descs[2].flags = 0;
	attachment_descs[2].format = present_format;
	attachment_descs[2].samples = msaa_count;
	attachment_descs[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachment_descs[2].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;  // if msaa count > 1, store op must be DONT_CARE
	attachment_descs[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment_descs[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachment_descs[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachment_descs[2].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	refs[2].attachment = 2;
	refs[2].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass_desc[1];
	subpass_desc[0].flags = 0;
	subpass_desc[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // Available: graphics | ray-tracing | compute
	subpass_desc[0].inputAttachmentCount = 0;
	subpass_desc[0].pInputAttachments = NULL;
	if (msaa_count > VK_SAMPLE_COUNT_1_BIT) {
		subpass_desc[0].colorAttachmentCount = 1;
		subpass_desc[0].pColorAttachments = &refs[2];
		subpass_desc[0].pResolveAttachments = &refs[0];
	}
	else {
		subpass_desc[0].colorAttachmentCount = 1;
		subpass_desc[0].pColorAttachments = &refs[0];
		subpass_desc[0].pResolveAttachments = NULL;
	}
	subpass_desc[0].pDepthStencilAttachment = &refs[1];
	subpass_desc[0].preserveAttachmentCount = 0;
	subpass_desc[0].pPreserveAttachments = NULL;

	VkSubpassDependency subpass_dep[1];
	subpass_dep[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	subpass_dep[0].dstSubpass = 0;
	subpass_dep[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	subpass_dep[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	subpass_dep[0].srcAccessMask = 0;
	subpass_dep[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	subpass_dep[0].dependencyFlags = 0;

	VkRenderPassCreateInfo ci;
	ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	ci.pNext = NULL;
	ci.flags = 0;
	ci.attachmentCount = msaa_count > VK_SAMPLE_COUNT_1_BIT ? 3 : 2;
	ci.pAttachments = attachment_descs;
	ci.subpassCount = sizeof(subpass_desc) / sizeof(*subpass_desc);
	ci.pSubpasses = subpass_desc;
	ci.dependencyCount = sizeof(subpass_dep) / sizeof(*subpass_dep);
	ci.pDependencies = subpass_dep;

	VkResult vk_err = vkCreateRenderPass(
		vk_driver->device.handle,
		&ci,
		vk_driver->vk_alc_cb_p,
		output);
	if (VK_SUCCESS != vk_err) {
		toy_err_vkerr(TOY_ERROR_CREATE_OBJECT_FAILED, vk_err, "vkCreateRenderPass failed", error);
		return;
	}

	toy_ok(error);
	return;
}


static void create_shader_triangle (
	toy_vulkan_driver_t* vk_driver,
	toy_vulkan_shader_loader_t* shader_loader,
	const struct toy_built_in_vulkan_pipeline_config_t* built_in_vk_pipeline_cfg,
	VkRenderPass render_pass,
	toy_vulkan_graphic_pipeline_t* output,
	toy_error_t* error)
{
	VkResult vk_err;
	VkDevice dev = vk_driver->device.handle;
	const VkAllocationCallbacks* vk_alc_cb = vk_driver->vk_alc_cb_p;

	output->desc_set_layouts[0] = VK_NULL_HANDLE;
	output->desc_set_layouts[1] = VK_NULL_HANDLE;
	output->desc_set_layouts[2] = VK_NULL_HANDLE;
	output->desc_set_layouts[3] = VK_NULL_HANDLE;

	VkPipelineLayout layout;
	VkPipelineLayoutCreateInfo layout_ci;
	layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layout_ci.pNext = NULL;
	layout_ci.flags = 0;
	layout_ci.setLayoutCount = 0;
	layout_ci.pSetLayouts = NULL;
	layout_ci.pushConstantRangeCount = 0;
	layout_ci.pPushConstantRanges = NULL;
	vk_err = vkCreatePipelineLayout(dev, &layout_ci, vk_alc_cb, &layout);
	if (VK_SUCCESS != vk_err) {
		toy_err_vkerr(TOY_ERROR_CREATE_OBJECT_FAILED, vk_err, "vkCreatePipelineLayout failed", error);
		goto FAIL_PIPELINE_LAYOUT;
	}
	output->layout = layout;

	VkShaderModule vertex_shader = toy_create_vulkan_shader_module("assets/SPIR_V/triangle_glsl_vt.spv", dev, shader_loader, vk_alc_cb, error);
	if (toy_is_failed(*error))
		goto FAIL_VERTEX_SHADER;
	VkShaderModule fragment_shader = toy_create_vulkan_shader_module("assets/SPIR_V/triangle_glsl_fg.spv", dev, shader_loader, vk_alc_cb, error);
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
	vertex_input.vertexBindingDescriptionCount = 1;
	vertex_input.pVertexBindingDescriptions = &built_in_vk_pipeline_cfg->vertex_input.built_in_vertex.binding_desc;
	vertex_input.vertexAttributeDescriptionCount = 3;
	vertex_input.pVertexAttributeDescriptions = built_in_vk_pipeline_cfg->vertex_input.built_in_vertex.attribute_descs;

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
	pipeline_ci.layout = layout;
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

	output->handle = pipeline_handle;

	toy_ok(error);
	return;

FAIL_PIPELINE:
	vkDestroyShaderModule(dev, fragment_shader, vk_alc_cb);
FAIL_FRAGMENT_SHADER:
	vkDestroyShaderModule(dev, vertex_shader, vk_alc_cb);
FAIL_VERTEX_SHADER:
	vkDestroyPipelineLayout(dev, layout, vk_alc_cb);
FAIL_PIPELINE_LAYOUT:
	return;
}


static void create_shader_mesh (
	toy_vulkan_driver_t* vk_driver,
	toy_vulkan_shader_loader_t* shader_loader,
	const struct toy_built_in_vulkan_pipeline_config_t* built_in_vk_pipeline_cfg,
	VkRenderPass render_pass,
	toy_vulkan_graphic_pipeline_t* output,
	toy_error_t* error)
{
	VkResult vk_err;
	VkDevice dev = vk_driver->device.handle;
	const VkAllocationCallbacks* vk_alc_cb = vk_driver->vk_alc_cb_p;

	vk_err = create_descriptor_set_layout_mvp(dev, vk_alc_cb, &output->desc_set_layouts[0]);
	if (VK_SUCCESS != vk_err) {
		toy_err_vkerr(TOY_ERROR_CREATE_OBJECT_FAILED, vk_err, "create_descriptor_set_layout_mvp failed", error);
		goto FAIL_DESC_SET_LAYOUT_MVP;
	}
	vk_err = create_descriptor_set_layout_single_texture(dev, vk_alc_cb, &output->desc_set_layouts[1]);
	if (VK_SUCCESS != vk_err) {
		toy_err_vkerr(TOY_ERROR_CREATE_OBJECT_FAILED, vk_err, "create_descriptor_set_layout_single_texture failed", error);
		goto FAIL_DESC_SET_LAYOUT_SINGLE_TEXTURE;
	}
	output->desc_set_layouts[2] = VK_NULL_HANDLE;
	output->desc_set_layouts[3] = VK_NULL_HANDLE;

	VkPipelineLayout layout;
	VkPipelineLayoutCreateInfo layout_ci;
	layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layout_ci.pNext = NULL;
	layout_ci.flags = 0;
	layout_ci.setLayoutCount = 2;
	layout_ci.pSetLayouts = output->desc_set_layouts;
	layout_ci.pushConstantRangeCount = 0;
	layout_ci.pPushConstantRanges = NULL;
	vk_err = vkCreatePipelineLayout(dev, &layout_ci, vk_alc_cb, &layout);
	if (VK_SUCCESS != vk_err) {
		toy_err_vkerr(TOY_ERROR_CREATE_OBJECT_FAILED, vk_err, "vkCreatePipelineLayout failed", error);
		goto FAIL_PIPELINE_LAYOUT;
	}
	output->layout = layout;

	VkShaderModule vertex_shader = toy_create_vulkan_shader_module("assets/SPIR_V/mesh_glsl_vt.spv", dev, shader_loader, vk_alc_cb, error);
	if (toy_is_failed(*error))
		goto FAIL_VERTEX_SHADER;
	VkShaderModule fragment_shader = toy_create_vulkan_shader_module("assets/SPIR_V/mesh_glsl_fg.spv", dev, shader_loader, vk_alc_cb, error);
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
	vertex_input.vertexBindingDescriptionCount = 1;
	vertex_input.pVertexBindingDescriptions = &built_in_vk_pipeline_cfg->vertex_input.built_in_vertex.binding_desc;
	vertex_input.vertexAttributeDescriptionCount = 3;
	vertex_input.pVertexAttributeDescriptions = built_in_vk_pipeline_cfg->vertex_input.built_in_vertex.attribute_descs;

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
	pipeline_ci.layout = layout;
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

	output->handle = pipeline_handle;

	toy_ok(error);
	return;

FAIL_PIPELINE:
	vkDestroyShaderModule(dev, fragment_shader, vk_alc_cb);
FAIL_FRAGMENT_SHADER:
	vkDestroyShaderModule(dev, vertex_shader, vk_alc_cb);
FAIL_VERTEX_SHADER:
	vkDestroyPipelineLayout(dev, layout, vk_alc_cb);
FAIL_PIPELINE_LAYOUT:
	vkDestroyDescriptorSetLayout(dev, output->desc_set_layouts[1], vk_alc_cb);
FAIL_DESC_SET_LAYOUT_SINGLE_TEXTURE:
	vkDestroyDescriptorSetLayout(dev, output->desc_set_layouts[0], vk_alc_cb);
FAIL_DESC_SET_LAYOUT_MVP:
	return;
}


static void create_shaders (
	toy_vulkan_driver_t* vk_driver,
	toy_vulkan_shader_loader_t* shader_loader,
	const struct toy_built_in_vulkan_pipeline_config_t* built_in_vk_pipeline_cfg,
	toy_vulkan_render_pass_main_camera_t* in_out,
	toy_error_t* error)
{
	uint32_t shader_count = 0;

	create_shader_triangle(vk_driver, shader_loader, built_in_vk_pipeline_cfg, in_out->handle, &in_out->shaders[0], error);
	if (toy_is_failed(*error))
		goto FAIL;
	++shader_count;

	create_shader_mesh(vk_driver, shader_loader, built_in_vk_pipeline_cfg, in_out->handle, &in_out->shaders[1], error);
	if (toy_is_failed(*error))
		goto FAIL;
	++shader_count;

	TOY_ASSERT(shader_count == sizeof(in_out->shaders) / sizeof(in_out->shaders[0]));

	toy_ok(error);
	return;

FAIL:
	for (uint32_t i = shader_count; i > 0; --i) {
		vkDestroyPipeline(vk_driver->device.handle, in_out->shaders[i - 1].handle, vk_driver->vk_alc_cb_p);
		vkDestroyPipelineLayout(vk_driver->device.handle, in_out->shaders[i - 1].layout, vk_driver->vk_alc_cb_p);
		for (uint32_t j = 0; j < 4; ++j) {
			if (VK_NULL_HANDLE != in_out->shaders[i-1].desc_set_layouts[j])
				vkDestroyDescriptorSetLayout(vk_driver->device.handle, in_out->shaders[i-1].desc_set_layouts[j], vk_driver->vk_alc_cb_p);
		}
	}
	return;
}


static void destroy_shaders (
	toy_vulkan_driver_t* vk_driver,
	toy_vulkan_render_pass_main_camera_t* pass)
{
	VkDevice dev = vk_driver->device.handle;
	const VkAllocationCallbacks* vk_alc_cb = vk_driver->vk_alc_cb_p;

	for (uint32_t i = sizeof(pass->shaders) / sizeof(pass->shaders[0]); i > 0; --i) {
		vkDestroyPipeline(dev, pass->shaders[i-1].handle, vk_alc_cb);
		vkDestroyPipelineLayout(dev, pass->shaders[i-1].layout, vk_alc_cb);
		for (uint32_t j = 0; j < 4; ++j) {
			if (VK_NULL_HANDLE != pass->shaders[i-1].desc_set_layouts[j])
				vkDestroyDescriptorSetLayout(vk_driver->device.handle, pass->shaders[i-1].desc_set_layouts[j], vk_driver->vk_alc_cb_p);
		}
	}
}


void toy_create_built_in_vulkan_render_pass_main_camera (
	toy_vulkan_driver_t* vk_driver,
	const toy_allocator_t* alc,
	toy_vulkan_shader_loader_t* shader_loader,
	const struct toy_built_in_vulkan_pipeline_config_t* built_in_vk_pipeline_cfg,
	toy_vulkan_render_pass_main_camera_t* output,
	toy_error_t* error)
{
	VkDevice dev = vk_driver->device.handle;
	const VkAllocationCallbacks* vk_alc_cb = vk_driver->vk_alc_cb_p;

	create_render_pass_handle(vk_driver, &output->handle, error);
	if (toy_is_failed(*error))
		goto FAIL_RENDER_PASS_HANDLE;

	create_framebuffers(vk_driver, output, alc, error);
	if (toy_is_failed(*error))
		goto FAIL_FRAMEBUFFER;

	create_shaders(vk_driver, shader_loader, built_in_vk_pipeline_cfg, output, error);
	if (toy_is_failed(*error))
		goto FAIL_SHADERS;

	toy_ok(error);
	return;

FAIL_SHADERS:
	destroy_framebuffers(vk_driver, output, alc);
FAIL_FRAMEBUFFER:
	vkDestroyRenderPass(dev, output->handle, vk_alc_cb);
FAIL_RENDER_PASS_HANDLE:
	return;
}


void toy_destroy_built_in_vulkan_render_pass_main_camera (
	toy_vulkan_driver_t* vk_driver,
	toy_vulkan_render_pass_main_camera_t* pass,
	const toy_allocator_t* alc)
{
	VkDevice dev = vk_driver->device.handle;
	const VkAllocationCallbacks* vk_alc_cb = vk_driver->vk_alc_cb_p;

	destroy_shaders(vk_driver, pass);

	destroy_framebuffers(vk_driver, pass, alc);

	vkDestroyRenderPass(dev, pass->handle, vk_alc_cb);
}


void toy_prepare_render_pass_main_camera (
	toy_built_in_vulkan_frame_resource_t* frame_res,
	toy_scene_t* scene,
	toy_vulkan_render_pass_main_camera_t* render_pass)
{
	struct toy_camera_view_project_matrix_t {
		toy_fmat4x4_t view;
		toy_fmat4x4_t project;
	};
	VkDeviceSize uniform_offset;
	uniform_offset = toy_vulkan_sub_buffer_alloc_L(
		&frame_res->uniform_stack,
		sizeof(toy_fmat4x4_t),
		sizeof(struct toy_camera_view_project_matrix_t),
		&render_pass->vp_buffer);
	TOY_ASSERT(VK_WHOLE_SIZE != uniform_offset);

	struct toy_camera_view_project_matrix_t* vp_mem = (struct toy_camera_view_project_matrix_t*)((uintptr_t)frame_res->mapping_memory + uniform_offset);
	TOY_ASSERT(scene->camera_count > 0);
	vp_mem->view = scene->cameras[0].view_matrix;
	vp_mem->project = scene->cameras[0].project_matrix;

	uint32_t model_matrix_count = 0;
	toy_scene_entity_chunk_descriptor_t* chunk_desc = scene->chunk_descs;
	while (NULL != chunk_desc) {
		ptrdiff_t mesh_offset = toy_get_scene_chunk_offset(chunk_desc, TOY_SCENE_COMPONENT_TYPE_MESH);
		ptrdiff_t location_offset = toy_get_scene_chunk_offset(chunk_desc, TOY_SCENE_COMPONENT_TYPE_LOCATION);
		if (0 == mesh_offset || 0 == location_offset) {
			chunk_desc = chunk_desc->next;
			continue;
		}

		toy_scene_entity_chunk_header_t* chunk = chunk_desc->first_chunk;
		while (NULL != chunk) {
			model_matrix_count += chunk->entity_count;
			chunk = chunk->next;
		}
		chunk_desc = chunk_desc->next;
	}

	uniform_offset = toy_vulkan_sub_buffer_alloc_L(
		&frame_res->uniform_stack,
		sizeof(toy_fmat4x4_t),
		sizeof(toy_fmat4x4_t) * model_matrix_count,
		&render_pass->m_buffer);
	TOY_ASSERT(VK_WHOLE_SIZE != uniform_offset);
	toy_fmat4x4_t* model_mem = (toy_fmat4x4_t*)((uintptr_t)frame_res->mapping_memory + uniform_offset);

	uint32_t model_index = 0;
	chunk_desc = scene->chunk_descs;
	while (NULL != chunk_desc) {
		ptrdiff_t mesh_offset = toy_get_scene_chunk_offset(chunk_desc, TOY_SCENE_COMPONENT_TYPE_MESH);
		ptrdiff_t location_offset = toy_get_scene_chunk_offset(chunk_desc, TOY_SCENE_COMPONENT_TYPE_LOCATION);
		if (0 == mesh_offset || 0 == location_offset) {
			chunk_desc = chunk_desc->next;
			continue;
		}

		toy_scene_entity_chunk_header_t* chunk = chunk_desc->first_chunk;
		while (NULL != chunk) {
			toy_fmat4x4_t* locations = (toy_fmat4x4_t*)((uintptr_t)chunk + location_offset);
			for (uint16_t i = 0; i < chunk->entity_count; ++i) {
				toy_fmat4x4_t model = locations[i];
				model_mem[model_index++] = model;// locations[i];
			}
			chunk = chunk->next;
		}
		chunk_desc = chunk_desc->next;
	}
}


static void draw_shader_0 (
	toy_built_in_vulkan_frame_resource_t* frame_res,
	toy_scene_t* scene,
	toy_vulkan_driver_t* vk_driver,
	toy_vulkan_render_pass_main_camera_t* render_pass,
	VkCommandBuffer draw_cmd)
{
	VkDevice dev = vk_driver->device.handle;

	vkCmdBindPipeline(draw_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, render_pass->shaders[0].handle);

	toy_scene_entity_chunk_descriptor_t* chunk_desc = scene->chunk_descs;
	while (NULL != chunk_desc) {
		ptrdiff_t mesh_offset = toy_get_scene_chunk_offset(chunk_desc, TOY_SCENE_COMPONENT_TYPE_MESH);
		ptrdiff_t location_offset = toy_get_scene_chunk_offset(chunk_desc, TOY_SCENE_COMPONENT_TYPE_LOCATION);
		if (0 == mesh_offset || 0 == location_offset) {
			chunk_desc = chunk_desc->next;
			continue;
		}

		toy_scene_entity_chunk_header_t* chunk = chunk_desc->first_chunk;
		while (NULL != chunk) {
			toy_asset_pool_item_ref_t* mesh_refs = (toy_asset_pool_item_ref_t*)((uintptr_t)chunk + mesh_offset);
			for (uint16_t i = 0; i < chunk->entity_count; ++i) {
				toy_mesh_t* mesh = (toy_mesh_t*)(toy_get_asset_item2(&mesh_refs[i]));
				toy_asset_pool_item_ref_t* primitive_ref = &mesh->first_primitive;
				while (NULL != primitive_ref) {
					toy_vulkan_mesh_primitive_p vk_mesh_primitive = toy_get_asset_item2(primitive_ref);
					primitive_ref = toy_get_next_asset_item_ref(mesh->ref_pool, primitive_ref);

					VkBuffer cmd_bind_buffers[] = { vk_mesh_primitive->vbo.handle };
					VkDeviceSize cmd_bind_buffer_offsets[] = { vk_mesh_primitive->vbo.offset };
					vkCmdBindVertexBuffers(draw_cmd, 0, 1, cmd_bind_buffers, cmd_bind_buffer_offsets);
					if (VK_NULL_HANDLE != vk_mesh_primitive->ibo.handle) {
						vkCmdBindIndexBuffer(
							draw_cmd,
							vk_mesh_primitive->ibo.handle,
							vk_mesh_primitive->ibo.offset,
							vk_mesh_primitive->index_type);
						vkCmdDrawIndexed(draw_cmd, vk_mesh_primitive->index_count, 1, 0, 0, 0);
					}
					else {
						vkCmdDraw(draw_cmd, vk_mesh_primitive->vertex_count, 1, 0, 0);
					}
				}
			}
			chunk = chunk->next;
		}
		chunk_desc = chunk_desc->next;
	}
}


static void draw_shader_1 (
	toy_built_in_vulkan_frame_resource_t* frame_res,
	toy_scene_t* scene,
	toy_vulkan_driver_t* vk_driver,
	toy_vulkan_render_pass_main_camera_t* render_pass,
	VkCommandBuffer draw_cmd)
{
	VkDevice dev = vk_driver->device.handle;
	VkResult vk_err;

	VkDescriptorSet mvp_desc_set;
	VkDescriptorSetLayout desc_set_layouts[] = {
		render_pass->shaders[1].desc_set_layouts[0],
	};
	VkDescriptorSetAllocateInfo desc_set_ai;
	desc_set_ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	desc_set_ai.pNext = NULL;
	desc_set_ai.descriptorPool = frame_res->descriptor_pool;
	desc_set_ai.descriptorSetCount = 1;
	desc_set_ai.pSetLayouts = desc_set_layouts;
	vk_err = vkAllocateDescriptorSets(dev, &desc_set_ai, &mvp_desc_set);
	TOY_ASSERT(VK_SUCCESS == vk_err);

	VkDescriptorBufferInfo buffer_info[2];
	VkWriteDescriptorSet desc_set_writes[2];
	buffer_info[0].buffer = render_pass->vp_buffer.handle;
	buffer_info[0].offset = render_pass->vp_buffer.offset;
	buffer_info[0].range = render_pass->vp_buffer.size;
	desc_set_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	desc_set_writes[0].pNext = NULL;
	desc_set_writes[0].dstSet = mvp_desc_set;
	desc_set_writes[0].dstBinding = 0;
	desc_set_writes[0].dstArrayElement = 0;
	desc_set_writes[0].descriptorCount = 1;
	desc_set_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	desc_set_writes[0].pImageInfo = NULL; // ignored with uniform buffer
	desc_set_writes[0].pBufferInfo = &buffer_info[0];
	desc_set_writes[0].pTexelBufferView = NULL; // ignored with uniform buffer

	buffer_info[1].buffer = render_pass->m_buffer.handle;
	buffer_info[1].offset = render_pass->m_buffer.offset;
	buffer_info[1].range = render_pass->m_buffer.size;
	desc_set_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	desc_set_writes[1].pNext = NULL;
	desc_set_writes[1].dstSet = mvp_desc_set;
	desc_set_writes[1].dstBinding = 1;
	desc_set_writes[1].dstArrayElement = 0;
	desc_set_writes[1].descriptorCount = 1;
	desc_set_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	desc_set_writes[1].pImageInfo = NULL; // ignored with uniform buffer
	desc_set_writes[1].pBufferInfo = &buffer_info[1];
	desc_set_writes[1].pTexelBufferView = NULL; // ignored with uniform buffer
	vkUpdateDescriptorSets(dev, 2, desc_set_writes, 0, NULL);

	vkCmdBindPipeline(draw_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, render_pass->shaders[1].handle);

	uint32_t model_index = 0;
	VkDeviceSize camera_buffer_offset = render_pass->vp_buffer.offset;
	toy_scene_entity_chunk_descriptor_t* chunk_desc = scene->chunk_descs;
	while (NULL != chunk_desc) {
		ptrdiff_t mesh_offset = toy_get_scene_chunk_offset(chunk_desc, TOY_SCENE_COMPONENT_TYPE_MESH);
		ptrdiff_t location_offset = toy_get_scene_chunk_offset(chunk_desc, TOY_SCENE_COMPONENT_TYPE_LOCATION);
		if (0 == mesh_offset || 0 == location_offset) {
			chunk_desc = chunk_desc->next;
			continue;
		}

		toy_scene_entity_chunk_header_t* chunk = chunk_desc->first_chunk;
		while (NULL != chunk) {
			toy_asset_pool_item_ref_t* mesh_refs = (toy_asset_pool_item_ref_t*)((uintptr_t)chunk + mesh_offset);
			for (uint16_t i = 0; i < chunk->entity_count; ++i) {
				toy_mesh_t* mesh = (toy_mesh_t*)(toy_get_asset_item2(&mesh_refs[i]));
				toy_asset_pool_item_ref_t* primitive_ref = &mesh->first_primitive;
				while (NULL != primitive_ref) {
					toy_vulkan_mesh_primitive_p vk_mesh_primitive = toy_get_asset_item2(primitive_ref);
					primitive_ref = toy_get_next_asset_item_ref(mesh->ref_pool, primitive_ref);

					VkDescriptorSet desc_sets[] = { mvp_desc_set };
					uint32_t dynamic_offsets[] = {
						(uint32_t)camera_buffer_offset,
						(uint32_t)sizeof(toy_fmat4x4_t) * model_index,
					};
					vkCmdBindDescriptorSets(
						draw_cmd,
						VK_PIPELINE_BIND_POINT_GRAPHICS,
						render_pass->shaders[1].layout,
						0, sizeof(desc_sets) / sizeof(*desc_sets), desc_sets,
						sizeof(dynamic_offsets) / sizeof(uint32_t), dynamic_offsets);

					VkBuffer cmd_bind_buffers[] = { vk_mesh_primitive->vbo.handle };
					VkDeviceSize cmd_bind_buffer_offsets[] = { vk_mesh_primitive->vbo.offset };
					vkCmdBindVertexBuffers(draw_cmd, 0, 1, cmd_bind_buffers, cmd_bind_buffer_offsets);
					if (VK_NULL_HANDLE != vk_mesh_primitive->ibo.handle) {
						vkCmdBindIndexBuffer(
							draw_cmd,
							vk_mesh_primitive->ibo.handle,
							vk_mesh_primitive->ibo.offset,
							vk_mesh_primitive->index_type);
						vkCmdDrawIndexed(draw_cmd, vk_mesh_primitive->index_count, 1, 0, 0, 0);
					}
					else {
						vkCmdDraw(draw_cmd, vk_mesh_primitive->vertex_count, 1, 0, 0);
					}
					++model_index;
				}
			}
			chunk = chunk->next;
		}
		chunk_desc = chunk_desc->next;
	}
}


void toy_draw_render_pass_main_camera (
	toy_built_in_vulkan_frame_resource_t* frame_res,
	toy_scene_t* scene,
	toy_vulkan_driver_t* vk_driver,
	toy_vulkan_render_pass_main_camera_t* render_pass,
	VkCommandBuffer draw_cmd,
	toy_error_t* error)
{
	VkClearColorValue clear_color = { 0.5f, 0.5f, 0.5f, 1.0f };
	VkClearDepthStencilValue clear_depth = { .depth = 1.0f, .stencil = 0 };
	VkClearValue clear_values[3];
	clear_values[0].color = clear_color;
	clear_values[1].depthStencil = clear_depth;
	uint32_t clear_value_count = 2;
	if (vk_driver->render_config.msaa_count > VK_SAMPLE_COUNT_1_BIT) {
		clear_values[2] = clear_values[0];
		clear_value_count = 3;
	}

	VkRenderPassBeginInfo render_pass_bi;
	render_pass_bi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_bi.pNext = NULL;
	render_pass_bi.renderPass = render_pass->handle;
	render_pass_bi.framebuffer = render_pass->frame_buffers[vk_driver->swapchain.current_image];
	render_pass_bi.renderArea.offset.x = 0;
	render_pass_bi.renderArea.offset.y = 0;
	render_pass_bi.renderArea.extent = vk_driver->swapchain.extent;
	render_pass_bi.clearValueCount = clear_value_count;
	render_pass_bi.pClearValues = clear_values;
	vkCmdBeginRenderPass(draw_cmd, &render_pass_bi, VK_SUBPASS_CONTENTS_INLINE);

	//draw_shader_0(frame_res, scene, vk_driver, render_pass, draw_cmd);
	draw_shader_1(frame_res, scene, vk_driver, render_pass, draw_cmd);

	vkCmdEndRenderPass(draw_cmd);

	toy_ok(error);
	return;
}
