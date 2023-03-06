#include "../include/auxiliary/toy_built_in_pipeline.h"

#include "../toy_assert.h"
#include <cstring>

TOY_EXTERN_C_START


static void allocate_pipeline_cmds (
	toy_vulkan_driver_t* vk_driver,
	toy_built_in_pipeline_t* pipeline,
	toy_error_t* error)
{
	VkResult vk_err;
	for (uint32_t i = 0; i < TOY_CONCURRENT_FRAME_MAX; ++i) {
		constexpr uint32_t cmd_count = sizeof(pipeline->cmds[i].draw_cmds) / sizeof(pipeline->cmds[i].draw_cmds[0]);
		VkCommandBufferAllocateInfo cmd_buffer_ai;
		cmd_buffer_ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmd_buffer_ai.pNext = NULL;
		cmd_buffer_ai.commandPool = pipeline->frame_res[i].graphic_cmd_pool;
		cmd_buffer_ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmd_buffer_ai.commandBufferCount = cmd_count;
		vk_err = vkAllocateCommandBuffers(vk_driver->device.handle, &cmd_buffer_ai, pipeline->cmds[i].draw_cmds);
		if (toy_unlikely(VK_SUCCESS != vk_err)) {
			toy_err_vkerr(TOY_ERROR_CREATE_OBJECT_FAILED, vk_err, "vkAllocateCommandBuffers failed", error);
			for (uint32_t j = i; j > 0; --j) {
				vkFreeCommandBuffers(
					vk_driver->device.handle,
					pipeline->frame_res[j].graphic_cmd_pool,
					cmd_count,
					pipeline->cmds[j].draw_cmds);
			}
			return;
		}
	}

	toy_ok(error);
	return;
}


void toy_create_built_in_vulkan_pipeline (
	toy_vulkan_driver_t* vk_driver,
	toy_memory_allocator_t* alc,
	toy_built_in_pipeline_t* output,
	toy_error_t* error)
{
	memset(output, 0, sizeof(*output));

	toy_init_built_in_vulkan_graphic_pipeline_config();

	auto pipeline_cfg = toy_get_built_in_vulkan_graphic_pipeline_config();
	toy_file_interface_t file_api = toy_std_file_interface();

	for (uint32_t i = 0; i < vk_driver->swapchain.frame_count; ++i) {
		toy_create_built_in_vulkan_frame_resource(
			&vk_driver->device,
			4 * 1024 * 1024,
			&vk_driver->vk_allocator,
			vk_driver->vk_alc_cb_p,
			&output->frame_res[i],
			error);
		if (toy_is_failed(*error)) {
			for (uint32_t j = i; j > 0; --j)
				toy_destroy_built_in_vulkan_frame_resource(&vk_driver->device, &vk_driver->vk_allocator, vk_driver->vk_alc_cb_p, &output->frame_res[j - 1]);
			goto FAIL_FRAME_RESOURCES;
		}
	}

	toy_vulkan_shader_loader_t shader_loader;
	shader_loader.file_api = &file_api;
	shader_loader.alc = &alc->list_alc;
	shader_loader.tmp_alc = &alc->buddy_alc;

	toy_create_built_in_vulkan_render_pass_main_camera(
		vk_driver,
		&alc->buddy_alc,
		&shader_loader,
		&output->built_in_desc_set_layout,
		&output->render_pass.main_camera,
		error);
	if (toy_is_failed(*error))
		goto FAIL_PASS_MAIN_CAMERA;

	allocate_pipeline_cmds(vk_driver, output, error);
	if (toy_is_failed(*error))
		goto FAIL_PASS_MAIN_CAMERA_CMD;

	toy_ok(error);
	return;

FAIL_PASS_MAIN_CAMERA_CMD:
	output->render_pass.main_camera.destroy_pass(
		vk_driver, &output->render_pass.main_camera, &alc->buddy_alc);
FAIL_PASS_MAIN_CAMERA:
	toy_destroy_built_in_vulkan_descriptor_set_layouts(
		&vk_driver->device, vk_driver->vk_alc_cb_p, &output->built_in_desc_set_layout);
	for (uint32_t i = vk_driver->swapchain.frame_count; i > 0; --i) {
		toy_destroy_built_in_vulkan_frame_resource(
			&vk_driver->device, &vk_driver->vk_allocator, vk_driver->vk_alc_cb_p, &output->frame_res[i - 1]);
	}
FAIL_FRAME_RESOURCES:
	return;
}


void toy_destroy_built_in_vulkan_pipeline (
	toy_vulkan_driver_t* vk_driver,
	toy_memory_allocator_t* alc,
	toy_built_in_pipeline_t* pipeline)
{
	// No need to free pipeline commands, vkDestroyCommandPool will do it implicitly

	pipeline->render_pass.main_camera.destroy_pass(
		vk_driver, &pipeline->render_pass.main_camera, &alc->buddy_alc);

	toy_destroy_built_in_vulkan_descriptor_set_layouts(
		&vk_driver->device, vk_driver->vk_alc_cb_p, &pipeline->built_in_desc_set_layout);

	for (uint32_t i = vk_driver->swapchain.frame_count; i > 0; --i) {
		toy_destroy_built_in_vulkan_frame_resource(
			&vk_driver->device, &vk_driver->vk_allocator, vk_driver->vk_alc_cb_p, &pipeline->frame_res[i - 1]);
	}
}


static void reset_frame_resource (
	toy_vulkan_driver_t* vk_driver,
	toy_built_in_vulkan_frame_resource_t* frame_res,
	toy_error_t* error)
{
	vkResetCommandPool(
		vk_driver->device.handle,
		frame_res->graphic_cmd_pool,
		VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);

	// descriptor sets are implicitly freed
	vkResetDescriptorPool(vk_driver->device.handle, frame_res->descriptor_pool, 0);

	toy_clear_vulkan_buffer_stack(&frame_res->uniform_stack);

	TOY_ASSERT(NULL == frame_res->mapping_memory);
	frame_res->mapping_memory = toy_map_vulkan_buffer_memory(
		vk_driver->device.handle,
		&frame_res->uniform_stack.buffer,
		error);
	TOY_ASSERT(NULL != frame_res->mapping_memory);
}


void toy_draw_scene (
	toy_vulkan_driver_t* vk_driver,
	toy_scene_t* scene,
	toy_asset_manager_t* asset_mgr,
	toy_built_in_pipeline_t* pipeline,
	toy_error_t* error)
{
	VkResult vk_err;
	uint32_t current_frame = vk_driver->swapchain.current_frame;
	toy_built_in_vulkan_frame_resource_t* frame_res = &pipeline->frame_res[current_frame];
	VkCommandBuffer draw_cmd = pipeline->cmds[current_frame].draw_cmds[0];

	reset_frame_resource(vk_driver, frame_res, error);
	if (toy_is_failed(*error))
		goto FAIL_RESET_FRAME_RESOURCE;

	pipeline->render_pass.main_camera.prepare(
		&pipeline->render_pass.main_camera, vk_driver, frame_res, scene, asset_mgr);

	toy_unmap_vulkan_buffer_memory(
		vk_driver->device.handle, &frame_res->uniform_stack.buffer, error);
	TODO_ASSERT(toy_is_ok(*error));
	frame_res->mapping_memory = NULL;

	VkCommandBufferBeginInfo cmd_buffer_bi;
	cmd_buffer_bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmd_buffer_bi.pNext = NULL;
	cmd_buffer_bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	cmd_buffer_bi.pInheritanceInfo = NULL;
	vk_err = vkBeginCommandBuffer(draw_cmd, &cmd_buffer_bi);
	if (VK_SUCCESS != vk_err) {
		toy_err_vkerr(TOY_ERROR_OPERATION_FAILED, vk_err, "vkBeginCommandBuffer failed", error);
		return;
	}

	pipeline->render_pass.main_camera.run(
		&pipeline->render_pass.main_camera,
		frame_res, scene, vk_driver, asset_mgr, &pipeline->built_in_desc_set_layout, draw_cmd, error);
	if (toy_is_failed(*error))
		goto FAIL_DRAW_PASS_MAIN_CAMERA;

	vkEndCommandBuffer(draw_cmd);

	VkSubmitInfo submit_info;
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pNext = NULL;
	submit_info.waitSemaphoreCount = 0;
	submit_info.pWaitSemaphores = NULL;
	submit_info.pWaitDstStageMask = NULL;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &draw_cmd;
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = &vk_driver->swapchain.finish_semaphores[vk_driver->swapchain.current_frame];
	vk_err = vkQueueSubmit(
		vk_driver->graphic_queue,
		1,
		&submit_info,
		vk_driver->swapchain.finish_fences[vk_driver->swapchain.current_frame]);

	toy_ok(error);
	return;

FAIL_DRAW_PASS_MAIN_CAMERA:
	vkEndCommandBuffer(draw_cmd);
FAIL_RESET_FRAME_RESOURCE:
	return;
}

TOY_EXTERN_C_END
