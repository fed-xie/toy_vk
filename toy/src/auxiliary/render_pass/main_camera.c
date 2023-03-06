#include "../../include/auxiliary/render_pass/main_camera.h"

#include "../../include/toy_file.h"
#include "../../include/toy_allocator.h"
#include "../../toy_assert.h"
#include <string.h>
#include "../shader/mesh.h"


static VkResult create_descriptor_set_layout (
	VkDevice dev,
	const VkAllocationCallbacks* vk_alc_cb,
	toy_vulkan_descriptor_set_layout_t* output)
{
	static const VkDescriptorSetLayoutBinding bindings[] = {
		{
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.pImmutableSamplers = NULL,
		}, {
			.binding = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.pImmutableSamplers = NULL,
		}, {
			.binding = 2,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.pImmutableSamplers = NULL,
		},{
			.binding = 3,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.pImmutableSamplers = NULL,
		},
	};
	static const binding_count = sizeof(bindings) / sizeof(bindings[0]);
	return toy_create_vulkan_descriptor_set_layout(dev, bindings, binding_count, vk_alc_cb, output);
}


static void create_framebuffers (
	toy_vulkan_driver_t* vk_driver,
	VkRenderPass handle,
	toy_vulkan_render_pass_context_main_camera_t* ctx,
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
		&ctx->depth_image, error);
	if (toy_is_failed(*error))
		return;

	ctx->frame_buffers = toy_alloc_aligned(alc, sizeof(VkFramebuffer) * vk_driver->swapchain.image_count, sizeof(VkFramebuffer));
	if (NULL == ctx->frame_buffers) {
		toy_err(TOY_ERROR_MEMORY_HOST_ALLOCATION_FAILED, "Failed to alloc VkFramebuffer", error);
		toy_destroy_vulkan_image(vk_driver->device.handle, &ctx->depth_image, vk_driver->vk_alc_cb_p);
		return;
	}

	for (uint32_t i = 0; i < vk_driver->swapchain.image_count; ++i) {
		VkImageView attachments[] = {
			vk_driver->swapchain.present_views[i],
			ctx->depth_image.view,
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
			vk_driver->device.handle, &framebuffer_ci, vk_driver->vk_alc_cb_p, &ctx->frame_buffers[i]);
		if (VK_SUCCESS != vk_err) {
			for (uint32_t j = i; j > 0; --j)
				vkDestroyFramebuffer(vk_driver->device.handle, ctx->frame_buffers[j-1], vk_driver->vk_alc_cb_p);
			toy_free_aligned(alc, ctx->frame_buffers);
			toy_destroy_vulkan_image(vk_driver->device.handle, &ctx->depth_image, vk_driver->vk_alc_cb_p);
			toy_err_vkerr(TOY_ERROR_CREATE_OBJECT_FAILED, vk_err, "vkCreateFramebuffer failed", error);
			return;
		}
	}

	toy_ok(error);
	return;
}


static void destroy_framebuffers (
	toy_vulkan_driver_t* vk_driver,
	toy_vulkan_render_pass_context_main_camera_t* ctx,
	const toy_allocator_t* alc)
{
	VkDevice dev = vk_driver->device.handle;
	const VkAllocationCallbacks* vk_alc_cb = vk_driver->vk_alc_cb_p;

	for (uint32_t i = vk_driver->swapchain.image_count; i > 0; --i)
		vkDestroyFramebuffer(dev, ctx->frame_buffers[i - 1], vk_alc_cb);
	toy_free_aligned(alc, ctx->frame_buffers);

	toy_destroy_vulkan_image(dev, &ctx->depth_image, vk_alc_cb);
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


static void create_shaders (
	toy_vulkan_driver_t* vk_driver,
	toy_vulkan_shader_loader_t* shader_loader,
	toy_built_in_vulkan_descriptor_set_layout_t* built_in_layouts,
	VkRenderPass pass_handle,
	toy_vulkan_render_pass_context_main_camera_t* ctx,
	const toy_allocator_t* alc,
	toy_error_t* error)
{
	ctx->shaders.mesh = toy_get_shader_module_mesh();
	
	toy_built_in_shader_module_t* shaders = (toy_built_in_shader_module_t*)(&ctx->shaders);
	const int shader_count = sizeof(ctx->shaders) / sizeof(toy_built_in_shader_module_t);

	int desc_set_idx = 0, layout_idx = 0, pipeline_idx = 0;
	for (; desc_set_idx < shader_count; ++desc_set_idx) {
		shaders[desc_set_idx].create_desc_set_layouts(vk_driver, built_in_layouts, error);
		if (toy_is_failed(*error))
			goto FAIL_DESC_SET_LAYOUTS;
	}

	for (; layout_idx < shader_count; ++layout_idx) {
		shaders[layout_idx].create_layout(vk_driver, built_in_layouts, &shaders[layout_idx].layout, error);
		if (toy_is_failed(*error))
			goto FAIL_LAYOUTS;
	}

	for (; pipeline_idx < shader_count; ++pipeline_idx) {
		shaders[pipeline_idx].create_module(
			vk_driver,
			shader_loader, toy_get_built_in_vulkan_graphic_pipeline_config(),
			shaders[pipeline_idx].layout, pass_handle,
			alc,
			&shaders[pipeline_idx],
			error);
		if (toy_is_failed(*error))
			goto FAIL_PIPELINE;
	}

	toy_ok(error);
	return;

FAIL_PIPELINE:
	for (int i = pipeline_idx; i > 0; --i)
		shaders[i - 1].destroy_module(&shaders[i - 1], vk_driver, alc);
FAIL_LAYOUTS:
	for (int i = layout_idx; i > 0; --i)
		vkDestroyPipelineLayout(vk_driver->device.handle, shaders[i - 1].layout.handle, vk_driver->vk_alc_cb_p);
FAIL_DESC_SET_LAYOUTS:
	// Do nothing, leave the descriptor sets clean work to upper level code
	return;
}


static void destroy_shaders (
	toy_vulkan_driver_t* vk_driver,
	const toy_allocator_t* alc,
	toy_vulkan_render_pass_context_main_camera_t* ctx)
{
	VkDevice dev = vk_driver->device.handle;
	const VkAllocationCallbacks* vk_alc_cb = vk_driver->vk_alc_cb_p;

	toy_built_in_shader_module_t* shaders = (toy_built_in_shader_module_t*)(&ctx->shaders);
	const int shader_count = sizeof(ctx->shaders) / sizeof(toy_built_in_shader_module_t);

	for (int i = shader_count; i > 0; --i) {
		shaders[i - 1].destroy_module(&shaders[i - 1], vk_driver, alc);
		vkDestroyPipelineLayout(dev, shaders[i - 1].layout.handle, vk_alc_cb);
		// Leave the descriptor sets clean work to upper level code
	}
}


static void destroy_render_pass (
	toy_vulkan_driver_t* vk_driver,
	toy_built_in_render_pass_module_t* pass,
	const toy_allocator_t* alc)
{
	VkDevice dev = vk_driver->device.handle;
	const VkAllocationCallbacks* vk_alc_cb = vk_driver->vk_alc_cb_p;

	toy_vulkan_render_pass_context_main_camera_t* ctx = pass->context;

	destroy_shaders(vk_driver, alc, ctx);

	destroy_framebuffers(vk_driver, ctx, alc);

	toy_free_aligned(alc, ctx);

	vkDestroyRenderPass(dev, pass->handle, vk_alc_cb);
}


static void prepare_camera (
	toy_vulkan_render_pass_context_main_camera_t* ctx,
	toy_built_in_vulkan_frame_resource_t* frame_res,
	toy_scene_t* scene)
{
	struct toy_camera_view_project_matrix_t {
		toy_fmat4x4_t view;
		toy_fmat4x4_t project;
	};
	VkDeviceSize uniform_offset = toy_vulkan_sub_buffer_alloc_L(
		&frame_res->uniform_stack,
		sizeof(toy_fmat4x4_t),
		sizeof(struct toy_camera_view_project_matrix_t),
		&ctx->vp_buffer);
	TOY_ASSERT(VK_WHOLE_SIZE != uniform_offset);

	struct toy_camera_view_project_matrix_t* vp_mem = (struct toy_camera_view_project_matrix_t*)((uintptr_t)frame_res->mapping_memory + uniform_offset);
	vp_mem->view = scene->main_camera.view_matrix;
	vp_mem->project = scene->main_camera.project_matrix;
}


static void prepare_model (
	toy_vulkan_render_pass_context_main_camera_t* ctx,
	toy_built_in_vulkan_frame_resource_t* frame_res,
	toy_scene_t* scene)
{
	VkDeviceSize uniform_offset = toy_vulkan_sub_buffer_alloc_L(
		&frame_res->uniform_stack,
		sizeof(toy_fmat4x4_t),
		sizeof(toy_fmat4x4_t) * scene->object_count,
		&ctx->m_buffer);
	TODO_ASSERT(VK_WHOLE_SIZE != uniform_offset);

	toy_fmat4x4_t* model_mem = (toy_fmat4x4_t*)((uintptr_t)frame_res->mapping_memory + uniform_offset);
	uint32_t model_index = 0;
	for (uint32_t i = 0; i < scene->object_count; ++i) {
		model_mem[model_index++] = scene->inst_matrices[i];
	}
}


static void prepare_instance (
	toy_vulkan_render_pass_context_main_camera_t* ctx,
	toy_built_in_vulkan_frame_resource_t* frame_res,
	toy_scene_t* scene,
	toy_asset_manager_t* asset_mgr)
{
	struct instance_data_t {
		uint32_t vertex_base;
		uint32_t instance_index;
	};
	VkDeviceSize uniform_offset = toy_vulkan_sub_buffer_alloc_L(
		&frame_res->uniform_stack,
		sizeof(struct instance_data_t),
		sizeof(struct instance_data_t) * scene->object_count,
		&ctx->inst_buffer);
	TODO_ASSERT(VK_WHOLE_SIZE != uniform_offset);

	struct instance_data_t* inst_mem = (struct instance_data_t*)((uintptr_t)frame_res->mapping_memory + uniform_offset);
	for (uint32_t i = 0; i < scene->object_count; ++i) {
		toy_vulkan_mesh_primitive_p vk_primitive = toy_get_asset_item(&asset_mgr->asset_pools.mesh_primitive, scene->mesh_primitives[i]);
		TOY_ASSERT(NULL != vk_primitive);

		inst_mem[i].vertex_base = vk_primitive->first_vertex;
		inst_mem[i].instance_index = i;
	}
}


static void prepare_render_pass (
	toy_built_in_render_pass_module_t* pass,
	toy_vulkan_driver_t* vk_driver,
	toy_built_in_vulkan_frame_resource_t* frame_res,
	toy_scene_t* scene,
	toy_asset_manager_t* asset_mgr)
{
	toy_vulkan_render_pass_context_main_camera_t* ctx = pass->context;

	prepare_camera(ctx, frame_res, scene);
	prepare_model(ctx, frame_res, scene);
	prepare_instance(ctx, frame_res, scene, asset_mgr);
}


static void update_descriptor_set (
	toy_built_in_render_pass_module_t* pass,
	toy_vulkan_driver_t* vk_driver,
	toy_built_in_vulkan_frame_resource_t* frame_res,
	toy_built_in_vulkan_descriptor_set_layout_t* built_in_desc_set_layouts,
	toy_scene_t* scene,
	toy_asset_manager_t* asset_mgr)
{
	toy_vulkan_render_pass_context_main_camera_t* ctx = pass->context;
	VkDevice dev = vk_driver->device.handle;
	VkDescriptorSetLayout desc_set_layouts[] = {
		built_in_desc_set_layouts->main_camera.handle,
	};
	VkDescriptorSetAllocateInfo desc_set_ai;
	desc_set_ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	desc_set_ai.pNext = NULL;
	desc_set_ai.descriptorPool = frame_res->descriptor_pool;
	desc_set_ai.descriptorSetCount = sizeof(desc_set_layouts) / sizeof(*desc_set_layouts);
	desc_set_ai.pSetLayouts = desc_set_layouts;
	VkResult vk_err = vkAllocateDescriptorSets(dev, &desc_set_ai, &ctx->desc_set);
	TODO_ASSERT(VK_SUCCESS == vk_err);
	VkDescriptorSet desc_set = ctx->desc_set;

	VkDescriptorBufferInfo buffer_info[4];
	VkWriteDescriptorSet desc_set_writes[4];
	buffer_info[0].buffer = asset_mgr->vk_private.vk_mesh_primitive_pool.vbo_pool.buffer.handle;
	buffer_info[0].offset = 0;
	buffer_info[0].range = asset_mgr->vk_private.vk_mesh_primitive_pool.vbo_pool.buffer.size;
	desc_set_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	desc_set_writes[0].pNext = NULL;
	desc_set_writes[0].dstSet = desc_set;
	desc_set_writes[0].dstBinding = 0;
	desc_set_writes[0].dstArrayElement = 0;
	desc_set_writes[0].descriptorCount = 1;
	desc_set_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	desc_set_writes[0].pImageInfo = NULL; // ignored with uniform buffer
	desc_set_writes[0].pBufferInfo = &buffer_info[0];
	desc_set_writes[0].pTexelBufferView = NULL; // ignored with uniform buffer

	buffer_info[1].buffer = ctx->vp_buffer.handle;
	buffer_info[1].offset = ctx->vp_buffer.offset;
	buffer_info[1].range = ctx->vp_buffer.size;
	desc_set_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	desc_set_writes[1].pNext = NULL;
	desc_set_writes[1].dstSet = desc_set;
	desc_set_writes[1].dstBinding = 1;
	desc_set_writes[1].dstArrayElement = 0;
	desc_set_writes[1].descriptorCount = 1;
	desc_set_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	desc_set_writes[1].pImageInfo = NULL; // ignored with uniform buffer
	desc_set_writes[1].pBufferInfo = &buffer_info[1];
	desc_set_writes[1].pTexelBufferView = NULL; // ignored with uniform buffer

	buffer_info[2].buffer = ctx->m_buffer.handle;
	buffer_info[2].offset = ctx->m_buffer.offset;
	buffer_info[2].range = ctx->m_buffer.size;
	desc_set_writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	desc_set_writes[2].pNext = NULL;
	desc_set_writes[2].dstSet = desc_set;
	desc_set_writes[2].dstBinding = 2;
	desc_set_writes[2].dstArrayElement = 0;
	desc_set_writes[2].descriptorCount = 1;
	desc_set_writes[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	desc_set_writes[2].pImageInfo = NULL; // ignored with uniform buffer
	desc_set_writes[2].pBufferInfo = &buffer_info[2];
	desc_set_writes[2].pTexelBufferView = NULL; // ignored with uniform buffer

	buffer_info[3].buffer = ctx->inst_buffer.handle;
	buffer_info[3].offset = ctx->inst_buffer.offset;
	buffer_info[3].range = ctx->inst_buffer.size;
	desc_set_writes[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	desc_set_writes[3].pNext = NULL;
	desc_set_writes[3].dstSet = desc_set;
	desc_set_writes[3].dstBinding = 3;
	desc_set_writes[3].dstArrayElement = 0;
	desc_set_writes[3].descriptorCount = 1;
	desc_set_writes[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	desc_set_writes[3].pImageInfo = NULL; // ignored with uniform buffer
	desc_set_writes[3].pBufferInfo = &buffer_info[3];
	desc_set_writes[3].pTexelBufferView = NULL; // ignored with uniform buffer

	vkUpdateDescriptorSets(dev, sizeof(desc_set_writes) / sizeof(*desc_set_writes), desc_set_writes, 0, NULL);
}


static void run_render_pass (
	toy_built_in_render_pass_module_t* pass,
	toy_built_in_vulkan_frame_resource_t* frame_res,
	toy_scene_t* scene,
	toy_vulkan_driver_t* vk_driver,
	toy_asset_manager_t* asset_mgr,
	toy_built_in_vulkan_descriptor_set_layout_t* built_in_desc_set_layouts,
	VkCommandBuffer draw_cmd,
	toy_error_t* error)
{
	toy_vulkan_render_pass_context_main_camera_t* ctx = pass->context;

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
	render_pass_bi.renderPass = pass->handle;
	render_pass_bi.framebuffer = ctx->frame_buffers[vk_driver->swapchain.current_image];
	render_pass_bi.renderArea.offset.x = 0;
	render_pass_bi.renderArea.offset.y = 0;
	render_pass_bi.renderArea.extent = vk_driver->swapchain.extent;
	render_pass_bi.clearValueCount = clear_value_count;
	render_pass_bi.pClearValues = clear_values;
	vkCmdBeginRenderPass(draw_cmd, &render_pass_bi, VK_SUBPASS_CONTENTS_INLINE);
	
	update_descriptor_set(pass, vk_driver, frame_res, built_in_desc_set_layouts, scene, asset_mgr);

	VkDescriptorSet desc_sets[] = { ctx->desc_set };
	vkCmdBindDescriptorSets(
		draw_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
		ctx->shaders.mesh.layout.handle,
		0, sizeof(desc_sets) / sizeof(*desc_sets), desc_sets,
		0, NULL);

	vkCmdBindPipeline(draw_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->shaders.mesh.pipeline);

	vkCmdBindIndexBuffer(
		draw_cmd,
		asset_mgr->vk_private.vk_mesh_primitive_pool.ibo_pool16.buffer.handle,
		0,
		VK_INDEX_TYPE_UINT16);

	uint32_t primitive_index = scene->mesh_primitives[0];
	uint32_t instance_count = 0;
	for (uint32_t i = 0; i < scene->object_count; ++i) {
		if (scene->mesh_primitives[i] == primitive_index) {
			++instance_count;
			continue;
		}
		else {
			toy_vulkan_mesh_primitive_t* vk_primitive = toy_get_asset_item(&asset_mgr->asset_pools.mesh_primitive, scene->mesh_primitives[i]);
			TOY_ASSERT(NULL != vk_primitive);
			vkCmdDrawIndexed(draw_cmd, instance_count, 1, vk_primitive->first_index, 0, 0);
			instance_count = 0;
			primitive_index = scene->mesh_primitives[i];
		}
	}

	if (instance_count > 0) {
		toy_vulkan_mesh_primitive_t* vk_primitive = toy_get_asset_item(&asset_mgr->asset_pools.mesh_primitive, primitive_index);
		TOY_ASSERT(NULL != vk_primitive);
		vkCmdDrawIndexed(draw_cmd, instance_count, instance_count, vk_primitive->first_index, 0, 0);
	}

	vkCmdEndRenderPass(draw_cmd);

	toy_ok(error);
	return;
}


void toy_create_built_in_vulkan_render_pass_main_camera (
	toy_vulkan_driver_t* vk_driver,
	const toy_allocator_t* alc,
	toy_vulkan_shader_loader_t* shader_loader,
	toy_built_in_vulkan_descriptor_set_layout_t* desc_set_layous,
	toy_built_in_render_pass_module_t* output,
	toy_error_t* error)
{
	VkDevice dev = vk_driver->device.handle;
	const VkAllocationCallbacks* vk_alc_cb = vk_driver->vk_alc_cb_p;

	toy_vulkan_render_pass_context_main_camera_t* ctx = toy_alloc_aligned(
		alc, sizeof(toy_vulkan_render_pass_context_main_camera_t), sizeof(void*));
	if (NULL == ctx) {
		toy_err(TOY_ERROR_MEMORY_HOST_ALLOCATION_FAILED, "Alloc render pass main camera context failed", error);
		goto FAIL_ALLOC_CONTEXT;
	}

	VkRenderPass handle = VK_NULL_HANDLE;
	create_render_pass_handle(vk_driver, &handle, error);
	if (toy_is_failed(*error))
		goto FAIL_RENDER_PASS_HANDLE;

	create_framebuffers(vk_driver, handle, ctx, alc, error);
	if (toy_is_failed(*error))
		goto FAIL_FRAMEBUFFER;

	VkResult vk_err = create_descriptor_set_layout(
		dev, vk_alc_cb, &desc_set_layous->main_camera);
	if (VK_SUCCESS != vk_err) {
		toy_err_vkerr(TOY_ERROR_CREATE_OBJECT_FAILED, vk_err,
			"Create main camera render pass descriptor set layout failed", error);
		goto FAIL_DESCRIPTOR_SET;
	}

	create_shaders(vk_driver, shader_loader, desc_set_layous, handle, ctx, alc, error);
	if (toy_is_failed(*error))
		goto FAIL_SHADERS;

	output->context = ctx;
	output->handle = handle;
	output->destroy_pass = destroy_render_pass;
	output->prepare = prepare_render_pass;
	output->run = run_render_pass;

	toy_ok(error);
	return;

FAIL_SHADERS:
	// Do nothing, leave the descriptor set layout to upper level code
FAIL_DESCRIPTOR_SET:
	destroy_framebuffers(vk_driver, ctx, alc);
FAIL_FRAMEBUFFER:
	vkDestroyRenderPass(dev, output->handle, vk_alc_cb);
FAIL_RENDER_PASS_HANDLE:
	toy_free_aligned(alc, ctx);
FAIL_ALLOC_CONTEXT:
	return;
}
