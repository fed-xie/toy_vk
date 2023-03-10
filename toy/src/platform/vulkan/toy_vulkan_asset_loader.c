#include "../../include/platform/vulkan/toy_vulkan_asset_loader.h"

#include "../../include/toy_log.h"
#include "../../toy_assert.h"


static void toy_create_vulkan_asset_loader_cmd (
	VkDevice dev,
	uint32_t family_index,
	const VkAllocationCallbacks* vk_alc_cb,
	VkCommandPool* in_out_pool,
	VkCommandBuffer* output_cmd,
	toy_error_t* error)
{
	TOY_ASSERT(NULL != in_out_pool);
	VkResult vk_err;

	VkCommandPool cmd_pool = *in_out_pool;
	if (VK_NULL_HANDLE == *in_out_pool) {
		VkCommandPoolCreateInfo cmd_pool_ci;
		cmd_pool_ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmd_pool_ci.pNext = NULL;
		cmd_pool_ci.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
		cmd_pool_ci.queueFamilyIndex = family_index;
		vk_err = vkCreateCommandPool(dev, &cmd_pool_ci, vk_alc_cb, &cmd_pool);
		if (toy_unlikely(VK_SUCCESS != vk_err)) {
			toy_err_vkerr(TOY_ERROR_CREATE_OBJECT_FAILED, vk_err, "vkCreateCommandPool for asset loader command pool failed", error);
			return;
		}
	}

	VkCommandBufferAllocateInfo cmd_buffer_ai;
	cmd_buffer_ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmd_buffer_ai.pNext = NULL;
	cmd_buffer_ai.commandPool = cmd_pool;
	cmd_buffer_ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmd_buffer_ai.commandBufferCount = 1;
	vk_err = vkAllocateCommandBuffers(dev, &cmd_buffer_ai, output_cmd);
	if (toy_unlikely(VK_SUCCESS != vk_err)) {
		toy_err_vkerr(TOY_ERROR_CREATE_OBJECT_FAILED, vk_err, "vkAllocateCommandBuffers for asset loader command failed", error);
		if (VK_NULL_HANDLE == *in_out_pool)
			vkDestroyCommandPool(dev, cmd_pool, vk_alc_cb);
		return;
	}

	if (VK_NULL_HANDLE == *in_out_pool)
		*in_out_pool = cmd_pool;

	toy_ok(error);
	return;
}


static void toy_create_vulkan_asset_loader_synchronized_objs (
	VkDevice dev,
	toy_vulkan_asset_loader_t* loader,
	const VkAllocationCallbacks* vk_alc_cb,
	toy_error_t* error)
{
	VkResult vk_err;
	VkFenceCreateInfo fence_ci;
	fence_ci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_ci.pNext = NULL;
	fence_ci.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	vk_err = vkCreateFence(dev, &fence_ci, vk_alc_cb, &loader->fence);
	if (VK_SUCCESS != vk_err) {
		toy_err_vkerr(TOY_ERROR_CREATE_OBJECT_FAILED, vk_err, "vkCreateFence for asset loader failed", error);
		return;
	}

	VkSemaphoreCreateInfo sem_ci;
	sem_ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	sem_ci.pNext = NULL;
	sem_ci.flags = 0;
	vk_err = vkCreateSemaphore(dev, &sem_ci, vk_alc_cb, &loader->semaphore);
	if (VK_SUCCESS != vk_err) {
		toy_err_vkerr(TOY_ERROR_CREATE_OBJECT_FAILED, vk_err, "vkCreateSemaphore for asset loader failed", error);
		vkDestroyFence(dev, loader->fence, vk_alc_cb);
		return;
	}

	loader->semaphore_wait_stage = 0;

	toy_ok(error);
	return;
}


void toy_create_vulkan_asset_loader (
	toy_vulkan_device_t* vk_device,
	VkDeviceSize uniform_size,
	toy_vulkan_memory_allocator_p vk_alc,
	toy_vulkan_asset_loader_t* output,
	toy_error_t* error)
{
	uint32_t transfer_family_index = vk_device->device_queue_families.transfer.family;
	uint32_t graphic_family_index = vk_device->device_queue_families.graphic.family;
	VkDevice dev = vk_device->handle;
	
	output->transfer_pool = VK_NULL_HANDLE;
	toy_create_vulkan_asset_loader_cmd(
		dev, transfer_family_index, vk_alc->vk_alc_cb_p, &output->transfer_pool, &output->transfer_cmd, error);
	if (toy_is_failed(*error))
		goto FAIL_TRANSFER_CMD;

	output->graphic_pool = (transfer_family_index == graphic_family_index ? output->transfer_pool : VK_NULL_HANDLE);
	toy_create_vulkan_asset_loader_cmd(
		dev, graphic_family_index, vk_alc->vk_alc_cb_p, &output->graphic_pool, &output->graphic_cmd, error);
	if (toy_is_failed(*error))
		goto FAIL_GRAPHIC_CMD;

	vkGetDeviceQueue(dev, transfer_family_index, vk_device->device_queue_families.transfer.offset, &output->transfer_queue);
	vkGetDeviceQueue(dev, graphic_family_index, vk_device->device_queue_families.graphic.offset, &output->graphic_queue);

	toy_create_vulkan_asset_loader_synchronized_objs(
		dev, output, vk_alc->vk_alc_cb_p, error);
	if (toy_is_failed(*error))
		goto FAIL_SYNC_OBJ;

	output->vk_alc = vk_alc;

	const VkMemoryPropertyFlags property_flags[] = {
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
	};
	const uint32_t flag_count = sizeof(property_flags) / sizeof(*property_flags);
	toy_create_vulkan_buffer(
		dev,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		property_flags, flag_count, uniform_size,
		&vk_device->physical_device.memory_properties,
		&vk_alc->vk_list_alc, vk_alc->vk_alc_cb_p,
		&output->stage_stack.buffer,
		error);
	if (toy_is_failed(*error))
		goto FAIL_STAGE_STACK;
	toy_init_vulkan_buffer_stack(&output->stage_stack.buffer, &output->stage_stack);
	output->mapping_memory = NULL;

	toy_ok(error);
	return;

FAIL_STAGE_STACK:
	vkDestroySemaphore(dev, output->semaphore, vk_alc->vk_alc_cb_p);
	vkDestroyFence(dev, output->fence, vk_alc->vk_alc_cb_p);
FAIL_SYNC_OBJ:
	if (output->graphic_pool != output->transfer_pool)
		vkDestroyCommandPool(dev, output->graphic_pool, vk_alc->vk_alc_cb_p);
FAIL_GRAPHIC_CMD:
	vkDestroyCommandPool(dev, output->transfer_pool, vk_alc->vk_alc_cb_p);
FAIL_TRANSFER_CMD:
	toy_log_error(error);
	return;
}


void toy_destroy_vulkan_asset_loader (
	VkDevice dev,
	toy_vulkan_asset_loader_t* loader)
{
	TOY_ASSERT(NULL == loader->mapping_memory);

	const VkAllocationCallbacks* vk_alc_cb = loader->vk_alc->vk_alc_cb_p;

	toy_destroy_vulkan_buffer(dev, &loader->stage_stack.buffer, vk_alc_cb);

	vkDestroySemaphore(dev, loader->semaphore, vk_alc_cb);
	vkDestroyFence(dev, loader->fence, vk_alc_cb);

	if (loader->transfer_pool != loader->graphic_pool)
		vkDestroyCommandPool(dev, loader->graphic_pool, vk_alc_cb);
	vkDestroyCommandPool(dev, loader->transfer_pool, vk_alc_cb);
}


void toy_reset_vulkan_asset_loader (
	VkDevice dev,
	toy_vulkan_asset_loader_t* loader,
	toy_error_t* error)
{
	VkResult vk_err;
	const uint64_t timeout = 500 * 1000 * 1000; // 500 ms
	vk_err = vkWaitForFences(dev, 1, &loader->fence, VK_TRUE, timeout);
	if (toy_unlikely(VK_SUCCESS != vk_err)) {
		// Todo: recreate command fences and semaphores
		toy_err_vkerr(TOY_ERROR_OPERATION_FAILED, vk_err, "vkWaitForFences failed when reset asset loader", error);
		return;
	}

	vk_err = vkResetCommandPool(dev, loader->transfer_pool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
	if (toy_unlikely(VK_SUCCESS != vk_err)) {
		toy_err_vkerr(TOY_ERROR_OPERATION_FAILED, vk_err, "vkResetCommandPool for transfer cmd pool failed when reset asset loader", error);
		return;
	}

	if (loader->graphic_pool != loader->transfer_pool) {
		vk_err = vkResetCommandPool(dev, loader->graphic_pool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
		if (toy_unlikely(VK_SUCCESS != vk_err)) {
			toy_err_vkerr(TOY_ERROR_OPERATION_FAILED, vk_err, "vkResetCommandPool for graphic cmd pool failed when reset asset loader", error);
			return;
		}
	}

	if (NULL != loader->mapping_memory) {
		toy_unmap_vulkan_buffer_memory(dev, &loader->stage_stack.buffer, error);
		if (toy_is_ok(*error))
			loader->mapping_memory = NULL;
	}

	toy_ok(error);
	return;
}


void toy_map_vulkan_stage_memory (
	VkDevice dev,
	toy_vulkan_asset_loader_t* loader,
	toy_error_t* error)
{
	TOY_ASSERT(NULL == loader->mapping_memory);

	loader->mapping_memory = toy_map_vulkan_buffer_memory(dev, &loader->stage_stack.buffer, error);
}


void toy_unmap_vulkan_stage_memory (
	VkDevice dev,
	toy_vulkan_asset_loader_t* loader,
	toy_error_t* error)
{
	TOY_ASSERT(NULL != loader->mapping_memory);

	toy_unmap_vulkan_buffer_memory(dev, &loader->stage_stack.buffer, error);
	if (toy_is_ok(*error))
		loader->mapping_memory = NULL;
}


void toy_clear_vulkan_stage_memory (toy_vulkan_asset_loader_t* loader)
{
	toy_clear_vulkan_buffer_stack(&loader->stage_stack);
}


void toy_copy_data_to_vulkan_stage_memory (
	toy_stage_data_block_t* data_blocks,
	uint32_t block_count,
	toy_vulkan_asset_loader_t* loader,
	toy_vulkan_sub_buffer_t* output_buffers,
	toy_error_t* error)
{
	size_t total_size = 0;
	for (uint32_t i = 0; i < block_count; ++i)
		total_size += data_blocks[i].size + data_blocks[i].alignment;
	if (loader->stage_stack.buffer.size < total_size) {
		// Todo: recreate stage buffer stack
		toy_err(TOY_ERROR_MEMORY_DEVICE_ALLOCATION_FAILED, "Loader stage stack too small", error);
		return;
	}

	TOY_ASSERT(NULL != loader->mapping_memory);

	for (uint32_t i = 0; i < block_count; ++i) {
		VkDeviceSize buffer_size = toy_vulkan_sub_buffer_alloc_L(
			&loader->stage_stack, data_blocks[i].alignment, data_blocks[i].size, &output_buffers[i]);
		if (VK_WHOLE_SIZE == buffer_size) {
			toy_err(TOY_ERROR_MEMORY_DEVICE_ALLOCATION_FAILED, "Loader stage stack too small", error);
			for (uint32_t j = i; j > 0; --j)
				toy_vulkan_sub_buffer_free_L(&loader->stage_stack, &output_buffers[j-1]);
			return;
		}

		uintptr_t start = (uintptr_t)loader->mapping_memory + output_buffers[i].offset + output_buffers[i].source->binding.offset;
		memcpy((void*)start, data_blocks[i].data, data_blocks[i].size);
	}

	toy_ok(error);
	return;
}


VkResult toy_start_vkcmd_stage_mesh_primitive (
	toy_vulkan_asset_loader_t* loader)
{
	VkCommandBufferBeginInfo cmd_bi;
	cmd_bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmd_bi.pNext = NULL;
	cmd_bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	cmd_bi.pInheritanceInfo = NULL;
	return vkBeginCommandBuffer(loader->transfer_cmd, &cmd_bi);
}


void toy_submit_vkcmd_stage_mesh_primitive (
	VkDevice dev,
	toy_vulkan_asset_loader_t* loader,
	toy_error_t* error)
{
	VkResult vk_err;

	vk_err = vkEndCommandBuffer(loader->transfer_cmd);
	if (toy_unlikely(VK_SUCCESS != vk_err)) {
		toy_err_vkerr(TOY_ERROR_OPERATION_FAILED, vk_err, "vkEndCommandBuffer for load mesh primitive failed", error);
		return;
	}

	vk_err = vkResetFences(dev, 1, &loader->fence);
	if (toy_unlikely(VK_SUCCESS != vk_err)) {
		toy_err_vkerr(TOY_ERROR_OPERATION_FAILED, vk_err, "vkResetFences for submit cmd of mesh primitive failed", error);
		return;
	}

	VkSubmitInfo transfer_submit_info;
	transfer_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	transfer_submit_info.pNext = NULL;
	transfer_submit_info.waitSemaphoreCount = 0;
	transfer_submit_info.pWaitSemaphores = NULL;
	transfer_submit_info.pWaitDstStageMask = NULL;
	transfer_submit_info.commandBufferCount = 1;
	transfer_submit_info.pCommandBuffers = &loader->transfer_cmd;
	transfer_submit_info.signalSemaphoreCount = 0;
	transfer_submit_info.pSignalSemaphores = NULL;
	vk_err = vkQueueSubmit(loader->transfer_queue, 1, &transfer_submit_info, loader->fence);
	if (toy_unlikely(VK_SUCCESS != vk_err)) {
		toy_err_vkerr(TOY_ERROR_OPERATION_FAILED, vk_err, "vkQueueSubmit for submit cmd of mesh primitive failed", error);
		return;
	}

	toy_ok(error);
	return;
}


VkResult toy_start_vkcmd_stage_image (
	toy_vulkan_asset_loader_t* loader)
{
	VkResult vk_err;
	VkCommandBufferBeginInfo cmd_bi;
	cmd_bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmd_bi.pNext = NULL;
	cmd_bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	cmd_bi.pInheritanceInfo = NULL;
	vk_err = vkBeginCommandBuffer(loader->transfer_cmd, &cmd_bi);
	if (VK_SUCCESS != vk_err)
		return vk_err;

	vk_err = vkBeginCommandBuffer(loader->graphic_cmd, &cmd_bi);
	if (VK_SUCCESS != vk_err) {
		vkEndCommandBuffer(loader->transfer_cmd);
		return vk_err;
	}
	return vk_err;
}


// vkspec.html#synchronization-pipeline-barriers
// vkspec.html#synchronization-memory-barriers
void toy_vkcmd_stage_texture_image (
	toy_vulkan_asset_loader_t* loader,
	toy_vulkan_sub_buffer_p src_buffer,
	toy_vulkan_image_p dst_image,
	uint32_t width,
	uint32_t height,
	uint32_t mipmap_level)
{
	VkImageMemoryBarrier transfer_mem_barrier;
	transfer_mem_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	transfer_mem_barrier.pNext = NULL;
	transfer_mem_barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
	transfer_mem_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	transfer_mem_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	transfer_mem_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	transfer_mem_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	transfer_mem_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	transfer_mem_barrier.image = dst_image->handle;
	transfer_mem_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	transfer_mem_barrier.subresourceRange.baseMipLevel = 0;
	transfer_mem_barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
	transfer_mem_barrier.subresourceRange.baseArrayLayer = 0;
	transfer_mem_barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
	vkCmdPipelineBarrier(
		loader->transfer_cmd,
		VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
		0,
		0, NULL,
		0, NULL,
		1, &transfer_mem_barrier);

	VkBufferImageCopy copy_regions[TOY_MAX_VULKAN_MIPMAP_LAVEL];
	TOY_ASSERT(TOY_MAX_VULKAN_MIPMAP_LAVEL >= mipmap_level);
	for (uint32_t mipmap_lv_i = 0; mipmap_lv_i < mipmap_level; ++mipmap_lv_i) {
		//VkImageSubresource img_sub_res;
		//img_sub_res.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		//img_sub_res.mipLevel = mipmap_lv_i;
		//img_sub_res.arrayLayer = 0;
		//VkSubresourceLayout sub_res_layout;
		//vkGetImageSubresourceLayout(dev, dst_image->handle, &img_sub_res, &sub_res_layout);

		copy_regions[mipmap_lv_i].bufferOffset = src_buffer->offset;
		copy_regions[mipmap_lv_i].bufferRowLength = width; // or 0 when tightly packed
		copy_regions[mipmap_lv_i].bufferImageHeight = height; // or 0 when tightly packed
		copy_regions[mipmap_lv_i].imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copy_regions[mipmap_lv_i].imageSubresource.mipLevel = mipmap_lv_i;
		copy_regions[mipmap_lv_i].imageSubresource.baseArrayLayer = 0;
		copy_regions[mipmap_lv_i].imageSubresource.layerCount = 1;
		copy_regions[mipmap_lv_i].imageOffset.x = 0;
		copy_regions[mipmap_lv_i].imageOffset.y = 0;
		copy_regions[mipmap_lv_i].imageOffset.z = 0;
		copy_regions[mipmap_lv_i].imageExtent.width = width;
		copy_regions[mipmap_lv_i].imageExtent.height = height;
		copy_regions[mipmap_lv_i].imageExtent.depth = 1;
	}

	vkCmdCopyBufferToImage(
		loader->transfer_cmd,
		src_buffer->handle,
		dst_image->handle,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		mipmap_level, copy_regions);

	VkImageMemoryBarrier fmt_mem_barrier;
	fmt_mem_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	fmt_mem_barrier.pNext = NULL;
	fmt_mem_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	fmt_mem_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	fmt_mem_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	fmt_mem_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	fmt_mem_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	fmt_mem_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	fmt_mem_barrier.image = dst_image->handle;
	fmt_mem_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	fmt_mem_barrier.subresourceRange.baseMipLevel = 0;
	fmt_mem_barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
	fmt_mem_barrier.subresourceRange.baseArrayLayer = 0;
	fmt_mem_barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
	vkCmdPipelineBarrier(
		loader->graphic_cmd,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		0,
		0, NULL,
		0, NULL,
		1, &fmt_mem_barrier);
}


void toy_submit_vkcmd_stage_image (
	VkDevice dev,
	toy_vulkan_asset_loader_t* loader,
	toy_error_t* error)
{
	VkResult vk_err;

	vk_err = vkEndCommandBuffer(loader->transfer_cmd);
	if (toy_unlikely(VK_SUCCESS != vk_err)) {
		toy_err_vkerr(TOY_ERROR_OPERATION_FAILED, vk_err, "vkEndCommandBuffer for transfer command failed", error);
		return;
	}

	vk_err = vkEndCommandBuffer(loader->graphic_cmd);
	if (toy_unlikely(VK_SUCCESS != vk_err)) {
		toy_err_vkerr(TOY_ERROR_OPERATION_FAILED, vk_err, "vkEndCommandBuffer for graphic command failed", error);
		return;
	}

	vk_err = vkResetFences(dev, 1, &loader->fence);
	if (toy_unlikely(VK_SUCCESS != vk_err)) {
		toy_err_vkerr(TOY_ERROR_OPERATION_FAILED, vk_err, "vkResetFences for submit cmd failed", error);
		return;
	}

	VkSubmitInfo submit_infos[2];
	submit_infos[0].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_infos[0].pNext = NULL;
	submit_infos[0].waitSemaphoreCount = 0;
	submit_infos[0].pWaitSemaphores = NULL;
	submit_infos[0].pWaitDstStageMask = NULL;
	submit_infos[0].commandBufferCount = 1;
	submit_infos[0].pCommandBuffers = &loader->transfer_cmd;
	submit_infos[0].signalSemaphoreCount = 1;
	submit_infos[0].pSignalSemaphores = &loader->semaphore;

	VkPipelineStageFlags wait_stages = VK_PIPELINE_STAGE_TRANSFER_BIT;
	submit_infos[1].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_infos[1].pNext = NULL;
	submit_infos[1].waitSemaphoreCount = 1;
	submit_infos[1].pWaitSemaphores = &loader->semaphore;
	submit_infos[1].pWaitDstStageMask = &wait_stages;
	submit_infos[1].commandBufferCount = 1;
	submit_infos[1].pCommandBuffers = &loader->graphic_cmd;
	submit_infos[1].signalSemaphoreCount = 0;
	submit_infos[1].pSignalSemaphores = NULL;

	if (loader->transfer_queue == loader->graphic_queue) {
		vk_err = vkQueueSubmit(loader->graphic_queue, 2, submit_infos, loader->fence);
		if (toy_unlikely(VK_SUCCESS != vk_err)) {
			toy_err_vkerr(TOY_ERROR_OPERATION_FAILED, vk_err, "vkQueueSubmit for submit cmd failed", error);
		}
		return;
	}
	else {
		vk_err = vkQueueSubmit(loader->transfer_queue, 1, &submit_infos[0], VK_NULL_HANDLE);
		if (toy_unlikely(VK_SUCCESS != vk_err)) {
			toy_err_vkerr(TOY_ERROR_OPERATION_FAILED, vk_err, "vkQueueSubmit for submit cmd failed", error);
			return;
		}
		vk_err = vkQueueSubmit(loader->graphic_queue, 1, &submit_infos[1], loader->fence);
		if (toy_unlikely(VK_SUCCESS != vk_err)) {
			toy_err_vkerr(TOY_ERROR_OPERATION_FAILED, vk_err, "vkQueueSubmit for submit cmd failed", error);
			return;
		}
	}

	toy_ok(error);
	return;
}
