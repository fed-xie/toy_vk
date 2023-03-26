#include "main_camera.h"

#include "../../include/toy_file.h"
#include "../../include/toy_allocator.h"
#include "../../toy_assert.h"
#include <string.h>
#include "../vulkan_pipeline/base.h"



static void prepare_camera (
	toy_built_in_vulkan_render_pass_context_t* ctx,
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
	toy_calc_camera_view_matrix(&scene->main_camera, &vp_mem->view);
	toy_calc_camera_project_matrix(&scene->main_camera, &vp_mem->project);
}


static void prepare_model (
	toy_built_in_vulkan_render_pass_context_t* ctx,
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
	toy_built_in_vulkan_render_pass_context_t* ctx,
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
	uint32_t last_mesh_index = UINT32_MAX;
	uint32_t vertex_base = 0;
	for (uint32_t i = 0; i < scene->object_count; ++i) {
		if (scene->meshes[i] != last_mesh_index) {
			toy_mesh_t* mesh = toy_get_asset_item(&asset_mgr->asset_pools.mesh, scene->meshes[i]);
			TOY_ASSERT(NULL != mesh && UINT32_MAX != mesh->primitive_index);
			toy_vulkan_mesh_primitive_p vk_primitive = toy_get_asset_item(&asset_mgr->asset_pools.mesh_primitive, mesh->primitive_index);
			TOY_ASSERT(NULL != vk_primitive);
			vertex_base = vk_primitive->first_index;
		}

		inst_mem[i].vertex_base = vertex_base;
		inst_mem[i].instance_index = i;
	}
}


void toy_prepare_render_pass_main_camera (
	toy_vulkan_driver_t* vk_driver,
	toy_built_in_pipeline_t* pipeline,
	toy_scene_t* scene,
	toy_asset_manager_t* asset_mgr)
{
	uint32_t current_frame = vk_driver->swapchain.current_frame;
	toy_built_in_vulkan_frame_resource_t* frame_res = &pipeline->frame_res[current_frame];

	prepare_camera(&pipeline->pass_context, frame_res, scene);
	prepare_model(&pipeline->pass_context, frame_res, scene);
	prepare_instance(&pipeline->pass_context, frame_res, scene, asset_mgr);
}


static void update_descriptor_set (
	toy_built_in_pipeline_t* pipeline,
	toy_vulkan_driver_t* vk_driver,
	toy_scene_t* scene,
	toy_asset_manager_t* asset_mgr)
{
	uint32_t current_frame = vk_driver->swapchain.current_frame;
	toy_built_in_vulkan_frame_resource_t* frame_res = &pipeline->frame_res[current_frame];

	toy_built_in_vulkan_render_pass_context_t* ctx = &pipeline->pass_context;
	VkDevice dev = vk_driver->device.handle;
	VkDescriptorSetLayout desc_set_layouts[] = {
		pipeline->desc_set_layouts.main_camera.handle,
	};
	VkDescriptorSetAllocateInfo desc_set_ai;
	desc_set_ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	desc_set_ai.pNext = NULL;
	desc_set_ai.descriptorPool = frame_res->descriptor_pool;
	desc_set_ai.descriptorSetCount = sizeof(desc_set_layouts) / sizeof(*desc_set_layouts);
	desc_set_ai.pSetLayouts = desc_set_layouts;
	VkResult vk_err = vkAllocateDescriptorSets(dev, &desc_set_ai, &ctx->mvp_desc_set);
	TODO_ASSERT(VK_SUCCESS == vk_err);
	VkDescriptorSet desc_set = ctx->mvp_desc_set;

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


static void draw_mesh (
	toy_built_in_pipeline_t* pipeline,
	VkCommandBuffer draw_cmd,
	toy_built_in_vulkan_descriptor_set_layout_t* built_in_desc_set_layouts,
	toy_built_in_vulkan_frame_resource_t* frame_res,
	toy_vulkan_driver_t* vk_driver,
	toy_asset_manager_t* asset_mgr,
	uint32_t mesh_index,
	uint32_t* last_material_index,
	uint32_t instance_count,
	uint32_t first_instance)
{
	toy_built_in_vulkan_render_pass_context_t* ctx = &pipeline->pass_context;

	toy_mesh_t* mesh = toy_get_asset_item(&asset_mgr->asset_pools.mesh, mesh_index);
	TOY_ASSERT(NULL != mesh && UINT32_MAX != mesh->primitive_index);
	if (*last_material_index != mesh->material_index && UINT32_MAX != mesh->material_index) {
		VkDescriptorSet desc_set;
		VkDescriptorSetLayout desc_set_layouts[] = {
			built_in_desc_set_layouts->single_texture.handle,
		};
		VkDescriptorSetAllocateInfo desc_set_ai;
		desc_set_ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		desc_set_ai.pNext = NULL;
		desc_set_ai.descriptorPool = frame_res->descriptor_pool;
		desc_set_ai.descriptorSetCount = sizeof(desc_set_layouts) / sizeof(*desc_set_layouts);
		desc_set_ai.pSetLayouts = desc_set_layouts;
		VkResult vk_err = vkAllocateDescriptorSets(vk_driver->device.handle, &desc_set_ai, &desc_set);
		TODO_ASSERT(VK_SUCCESS == vk_err);

		toy_vulkan_descriptor_set_data_header_t** material_p = toy_get_asset_item(&asset_mgr->asset_pools.material, mesh->material_index);
		TOY_ASSERT(NULL != material_p);
		toy_update_vulkan_descriptor_set(vk_driver->device.handle, desc_set, *material_p);

		VkDescriptorSet desc_sets[] = { desc_set };
		vkCmdBindDescriptorSets(
			draw_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipeline->pipelline_layouts.mesh.handle,
			1, sizeof(desc_sets) / sizeof(*desc_sets), desc_sets,
			0, NULL);
		*last_material_index = mesh->material_index;
	}
	toy_vulkan_mesh_primitive_p vk_primitive = toy_get_asset_item(&asset_mgr->asset_pools.mesh_primitive, mesh->primitive_index);
	TOY_ASSERT(NULL != vk_primitive);
	vkCmdDrawIndexed(draw_cmd, vk_primitive->index_count, instance_count, vk_primitive->first_index, 0, first_instance);
}


void toy_run_render_pass_main_camera (
	toy_built_in_pipeline_t* pipeline,
	toy_built_in_vulkan_frame_resource_t* frame_res,
	toy_scene_t* scene,
	toy_vulkan_driver_t* vk_driver,
	toy_asset_manager_t* asset_mgr,
	toy_built_in_vulkan_descriptor_set_layout_t* built_in_desc_set_layouts,
	VkCommandBuffer draw_cmd,
	toy_error_t* error)
{
	toy_built_in_vulkan_render_pass_context_t* ctx = &pipeline->pass_context;

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
	render_pass_bi.renderPass = pipeline->render_passes.main_camera;
	render_pass_bi.framebuffer = ctx->camera_framebuffers[vk_driver->swapchain.current_image];
	render_pass_bi.renderArea.offset.x = 0;
	render_pass_bi.renderArea.offset.y = 0;
	render_pass_bi.renderArea.extent = vk_driver->swapchain.extent;
	render_pass_bi.clearValueCount = clear_value_count;
	render_pass_bi.pClearValues = clear_values;
	vkCmdBeginRenderPass(draw_cmd, &render_pass_bi, VK_SUBPASS_CONTENTS_INLINE);
	
	update_descriptor_set(pipeline, vk_driver, scene, asset_mgr);
	
	VkDescriptorSet desc_sets[] = { ctx->mvp_desc_set };
	vkCmdBindDescriptorSets(
		draw_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipeline->pipelline_layouts.mesh.handle,
		0, sizeof(desc_sets) / sizeof(*desc_sets), desc_sets,
		0, NULL);
	
	vkCmdBindPipeline(draw_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipelines.mesh);

	vkCmdBindIndexBuffer(
		draw_cmd,
		asset_mgr->vk_private.vk_mesh_primitive_pool.ibo_pool16.buffer.handle,
		0,
		VK_INDEX_TYPE_UINT16);

	uint32_t obj_i;
	uint32_t last_inst;
	uint32_t last_material_index = UINT32_MAX;
	uint32_t last_mesh = scene->meshes[0];
	uint32_t instance_count = 0;
	for (obj_i = 0, last_inst = obj_i; obj_i < scene->object_count; ++obj_i) {
		if (scene->meshes[obj_i] == last_mesh) {
			++instance_count;
			continue;
		}
		draw_mesh(pipeline, draw_cmd, built_in_desc_set_layouts, frame_res, vk_driver, asset_mgr, last_mesh, &last_material_index, instance_count, last_inst);
		last_mesh = scene->meshes[obj_i];
		instance_count = 1;
		last_inst = obj_i;
	}

	draw_mesh(pipeline, draw_cmd, built_in_desc_set_layouts, frame_res, vk_driver, asset_mgr, last_mesh, &last_material_index, instance_count, last_inst);

	vkCmdEndRenderPass(draw_cmd);

	toy_ok(error);
	return;
}
