#include "render_pass.h"


static void create_main_camera (
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

	VkSubpassDependency subpass_dep[2];
	subpass_dep[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	subpass_dep[0].dstSubpass = 0;
	subpass_dep[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpass_dep[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpass_dep[0].srcAccessMask = 0;
	subpass_dep[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpass_dep[0].dependencyFlags = 0;
	subpass_dep[1].srcSubpass = VK_SUBPASS_EXTERNAL;
	subpass_dep[1].dstSubpass = 0;
	subpass_dep[1].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	subpass_dep[1].srcAccessMask = 0;
	subpass_dep[1].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	subpass_dep[1].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	subpass_dep[1].dependencyFlags = 0;

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


static void create_shadow (
	toy_vulkan_driver_t* vk_driver,
	VkRenderPass* output,
	toy_error_t* error)
{
	VkFormat depth_format = vk_driver->render_config.depth_format;
	VkSampleCountFlagBits msaa_count = VK_SAMPLE_COUNT_1_BIT;

	VkAttachmentReference refs[1];
	VkAttachmentDescription attachment_descs[1];

	// depth-stencil attachment
	attachment_descs[0].flags = 0;
	attachment_descs[0].format = depth_format;
	attachment_descs[0].samples = msaa_count;
	attachment_descs[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachment_descs[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE; // if msaa count > 1, store op must be DONT_CARE
	attachment_descs[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment_descs[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachment_descs[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachment_descs[0].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	refs[0].attachment = 0;
	refs[0].layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass_desc[1];
	subpass_desc[0].flags = 0;
	subpass_desc[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // Available: graphics | ray-tracing | compute
	subpass_desc[0].inputAttachmentCount = 0;
	subpass_desc[0].pInputAttachments = NULL;
	subpass_desc[0].colorAttachmentCount = 0;
	subpass_desc[0].pColorAttachments = NULL;
	subpass_desc[0].pResolveAttachments = NULL;
	subpass_desc[0].pDepthStencilAttachment = &refs[0];
	subpass_desc[0].preserveAttachmentCount = 0;
	subpass_desc[0].pPreserveAttachments = NULL;

	VkSubpassDependency subpass_dep[1];
	subpass_dep[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	subpass_dep[0].dstSubpass = 0;
	subpass_dep[0].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	subpass_dep[0].srcAccessMask = 0;
	subpass_dep[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	subpass_dep[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	subpass_dep[0].dependencyFlags = 0;

	VkRenderPassCreateInfo ci;
	ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	ci.pNext = NULL;
	ci.flags = 0;
	ci.attachmentCount = sizeof(attachment_descs) / sizeof(*attachment_descs);
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



void toy_create_built_in_vulkan_render_passes (
	toy_vulkan_driver_t* vk_driver,
	const toy_allocator_t* alc,
	toy_built_in_vulkan_render_passes_t* output,
	toy_error_t* error)
{
	memset(output, 0, sizeof(*output));

	create_main_camera(vk_driver, &output->main_camera, error);
	if (toy_is_failed(*error))
		goto FAIL;

	create_shadow(vk_driver, &output->shadow, error);
	if (toy_is_failed(*error))
		goto FAIL;

	toy_ok(error);
	return;
FAIL:
	toy_destroy_built_in_vulkan_render_passes(vk_driver, alc, output);
	return;
}


void toy_destroy_built_in_vulkan_render_passes (
	toy_vulkan_driver_t* vk_driver,
	const toy_allocator_t* alc,
	toy_built_in_vulkan_render_passes_t* passes)
{
	VkRenderPass* render_passes = (VkRenderPass*)passes;
	const int pass_count = sizeof(*passes) / sizeof(VkRenderPass);

	VkDevice dev = vk_driver->device.handle;
	const VkAllocationCallbacks* vk_alc_cb = vk_driver->vk_alc_cb_p;

	for (int i = 0; i < pass_count; ++i) {
		if (VK_NULL_HANDLE != render_passes[i])
			vkDestroyRenderPass(dev, render_passes[i], vk_alc_cb);
	}
}



static void create_framebuffers (
	toy_vulkan_driver_t* vk_driver,
	VkRenderPass handle,
	toy_built_in_vulkan_render_pass_context_t* ctx,
	const toy_allocator_t* alc,
	toy_error_t* error)
{
	VkResult vk_err;

	toy_create_vulkan_image_depth(
		&vk_driver->vk_allocator,
		vk_driver->render_config.depth_format,
		VK_SAMPLE_COUNT_1_BIT,
		vk_driver->swapchain.extent.width,
		vk_driver->swapchain.extent.height,
		&ctx->camera_depth_image, error);
	if (toy_is_failed(*error))
		return;

	ctx->camera_framebuffers = toy_alloc_aligned(alc, sizeof(VkFramebuffer) * vk_driver->swapchain.image_count, sizeof(VkFramebuffer));
	if (NULL == ctx->camera_framebuffers) {
		toy_err(TOY_ERROR_MEMORY_HOST_ALLOCATION_FAILED, "Failed to alloc VkFramebuffer", error);
		toy_destroy_vulkan_image(vk_driver->device.handle, &ctx->camera_depth_image, vk_driver->vk_alc_cb_p);
		return;
	}

	for (uint32_t i = 0; i < vk_driver->swapchain.image_count; ++i) {
		VkImageView attachments[] = {
			vk_driver->swapchain.present_views[i],
			ctx->camera_depth_image.view,
		};
		VkFramebufferCreateInfo framebuffer_ci;
		framebuffer_ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_ci.pNext = NULL;
		framebuffer_ci.flags = 0;
		framebuffer_ci.renderPass = handle;
		framebuffer_ci.attachmentCount = 2;
		framebuffer_ci.pAttachments = attachments;
		framebuffer_ci.width = vk_driver->swapchain.extent.width;
		framebuffer_ci.height = vk_driver->swapchain.extent.height;
		framebuffer_ci.layers = 1;
		vk_err = vkCreateFramebuffer(
			vk_driver->device.handle, &framebuffer_ci, vk_driver->vk_alc_cb_p, &ctx->camera_framebuffers[i]);
		if (VK_SUCCESS != vk_err) {
			for (uint32_t j = i; j > 0; --j)
				vkDestroyFramebuffer(vk_driver->device.handle, ctx->camera_framebuffers[j - 1], vk_driver->vk_alc_cb_p);
			toy_free_aligned(alc, ctx->camera_framebuffers);
			toy_destroy_vulkan_image(vk_driver->device.handle, &ctx->camera_depth_image, vk_driver->vk_alc_cb_p);
			toy_err_vkerr(TOY_ERROR_CREATE_OBJECT_FAILED, vk_err, "vkCreateFramebuffer failed", error);
			return;
		}
	}

	toy_ok(error);
	return;
}


static void destroy_framebuffers (
	toy_vulkan_driver_t* vk_driver,
	toy_built_in_vulkan_render_pass_context_t* ctx,
	const toy_allocator_t* alc)
{
	VkDevice dev = vk_driver->device.handle;
	const VkAllocationCallbacks* vk_alc_cb = vk_driver->vk_alc_cb_p;

	for (uint32_t i = vk_driver->swapchain.image_count; i > 0; --i)
		vkDestroyFramebuffer(dev, ctx->camera_framebuffers[i - 1], vk_alc_cb);
	toy_free_aligned(alc, ctx->camera_framebuffers);

	toy_destroy_vulkan_image(dev, &ctx->camera_depth_image, vk_alc_cb);
}


void toy_create_built_in_render_pass_context (
	toy_vulkan_driver_t* vk_driver,
	toy_built_in_vulkan_render_passes_t* passes,
	const toy_allocator_t* alc,
	toy_built_in_vulkan_render_pass_context_t* output,
	toy_error_t* error)
{
	create_framebuffers(vk_driver, passes->main_camera, output, alc, error);
}


void toy_destroy_built_in_render_pass_context (
	toy_vulkan_driver_t* vk_driver,
	toy_built_in_vulkan_render_pass_context_t* ctx,
	const toy_allocator_t* alc)
{
	destroy_framebuffers(vk_driver, ctx, alc);
}
