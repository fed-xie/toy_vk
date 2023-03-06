#include "../../include/platform/vulkan/toy_vulkan_asset.h"

#include "../../toy_assert.h"
#include "../../include/toy_log.h"
#include <string.h>


static void create_vertex_buffer (
	toy_vulkan_memory_allocator_p vk_allocator,
	VkDeviceSize size,
	toy_vulkan_buffer_list_pool_t* output,
	toy_error_t* error)
{
	const VkMemoryPropertyFlags property_flags[] = {
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		0
	};
	const uint32_t flag_count = sizeof(property_flags) / sizeof(*property_flags);
	toy_create_vulkan_buffer(
		vk_allocator->device,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		property_flags, flag_count, size,
		&vk_allocator->memory_properties,
		&vk_allocator->vk_list_alc,
		vk_allocator->vk_alc_cb_p,
		&output->buffer,
		error);
	if (toy_unlikely(toy_is_failed(*error)))
		goto FAIL_CREATE_BUFFER;
	output->buffer.alignment = 0;

	TOY_ASSERT(sizeof(toy_vulkan_buffer_list_chunk_t) == sizeof(toy_vulkan_memory_list_chunk_t));
	toy_create_vulkan_buffer_list_pool(&output->buffer, &vk_allocator->chunk_alc, output, error);
	if (toy_unlikely(toy_is_failed(*error)))
		goto FAIL_CREATE_BUFFER_POOL;

	toy_ok(error);
	return;

FAIL_CREATE_BUFFER_POOL:
	vkDestroyBuffer(vk_allocator->device, output->buffer.handle, vk_allocator->vk_alc_cb_p);
FAIL_CREATE_BUFFER:
	return;
}


static void create_index_buffer (
	toy_vulkan_memory_allocator_p vk_allocator,
	VkDeviceSize size,
	toy_vulkan_buffer_list_pool_t* output,
	toy_error_t* error)
{
	if (0 == size) {
		memset(output, 0, sizeof(*output));
		toy_ok(error);
		return;
	}

	const VkMemoryPropertyFlags property_flags[] = {
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		0
	};
	const uint32_t flag_count = sizeof(property_flags) / sizeof(*property_flags);
	toy_create_vulkan_buffer(
		vk_allocator->device,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		property_flags, flag_count, size,
		&vk_allocator->memory_properties,
		&vk_allocator->vk_list_alc,
		vk_allocator->vk_alc_cb_p,
		&output->buffer,
		error);
	if (toy_unlikely(toy_is_failed(*error)))
		goto FAIL_CREATE_BUFFER;
	output->buffer.alignment = 0;

	TOY_ASSERT(sizeof(toy_vulkan_buffer_list_chunk_t) == sizeof(toy_vulkan_memory_list_chunk_t));
	toy_create_vulkan_buffer_list_pool(&output->buffer, &vk_allocator->chunk_alc, output, error);
	if (toy_unlikely(toy_is_failed(*error)))
		goto FAIL_CREATE_BUFFER_POOL;

	toy_ok(error);
	return;

FAIL_CREATE_BUFFER_POOL:
	vkDestroyBuffer(vk_allocator->device, output->buffer.handle, vk_allocator->vk_alc_cb_p);
FAIL_CREATE_BUFFER:
	return;
}


static void destroy_buffer (
	toy_vulkan_memory_allocator_p vk_allocator,
	toy_vulkan_buffer_list_pool_t* pool)
{
	if (VK_NULL_HANDLE != pool->buffer.handle) {
		vkDestroyBuffer(vk_allocator->device, pool->buffer.handle, vk_allocator->vk_alc_cb_p);
		toy_destroy_vulkan_buffer_list_pool(pool);
	}
}


void toy_create_vulkan_mesh_primitive_asset_pool (
	VkDeviceSize vertex_buffer_size,
	VkDeviceSize index_buffer8_size,
	VkDeviceSize index_buffer16_size,
	VkDeviceSize index_buffer32_size,
	toy_vulkan_memory_allocator_p vk_allocator,
	toy_vulkan_mesh_primitive_asset_pool_t* output,
	toy_error_t* error)
{
	create_vertex_buffer(vk_allocator, vertex_buffer_size, &output->vbo_pool, error);
	if (toy_is_failed(*error))
		goto FAIL_VERTEX_BUFFER;

	create_index_buffer(vk_allocator, index_buffer8_size, &output->ibo_pool8, error);
	if (toy_is_failed(*error))
		goto FAIL_INDEX_BUFFER8;

	create_index_buffer(vk_allocator, index_buffer16_size, &output->ibo_pool16, error);
	if (toy_is_failed(*error))
		goto FAIL_INDEX_BUFFER16;

	create_index_buffer(vk_allocator, index_buffer32_size, &output->ibo_pool32, error);
	if (toy_is_failed(*error))
		goto FAIL_INDEX_BUFFER32;

	output->vk_allocator = vk_allocator;

	toy_ok(error);
	return;
FAIL_INDEX_BUFFER32:
	destroy_buffer(vk_allocator, &output->ibo_pool16);
FAIL_INDEX_BUFFER16:
	destroy_buffer(vk_allocator, &output->ibo_pool8);
FAIL_INDEX_BUFFER8:
	destroy_buffer(vk_allocator, &output->vbo_pool);
FAIL_VERTEX_BUFFER:
	return;
}


void toy_destroy_vulkan_mesh_primitive_asset_pool (
	toy_vulkan_mesh_primitive_asset_pool_t* vk_pool)
{
	destroy_buffer(vk_pool->vk_allocator, &vk_pool->ibo_pool32);
	destroy_buffer(vk_pool->vk_allocator, &vk_pool->ibo_pool16);
	destroy_buffer(vk_pool->vk_allocator, &vk_pool->ibo_pool8);
	destroy_buffer(vk_pool->vk_allocator, &vk_pool->vbo_pool);
}


void toy_vulkan_look_up_vertex_sub_buffer (
	toy_vulkan_mesh_primitive_asset_pool_t* vk_pool,
	toy_vulkan_mesh_primitive_t* primitive,
	toy_vulkan_sub_buffer_t* output_vbo)
{
	output_vbo->handle = vk_pool->vbo_pool.buffer.handle;
	output_vbo->offset = primitive->first_vertex * primitive->vertex_stride;
	output_vbo->size = primitive->vertex_stride * primitive->vertex_count;
	output_vbo->padding = 0;
	output_vbo->source = &vk_pool->vbo_pool.buffer;
}


void toy_vulkan_look_up_index_sub_buffer (
	toy_vulkan_mesh_primitive_asset_pool_t* vk_pool,
	toy_vulkan_mesh_primitive_t* primitive,
	toy_vulkan_sub_buffer_t* output_ibo)
{
	toy_vulkan_buffer_t* buffer = NULL;
	if (sizeof(uint16_t) == primitive->index_stride)
		buffer = &vk_pool->ibo_pool16.buffer;
	else if (sizeof(uint8_t) == primitive->index_stride)
		buffer = &vk_pool->ibo_pool8.buffer;
	else if (sizeof(uint32_t) == primitive->index_stride)
		buffer = &vk_pool->ibo_pool32.buffer;
	else {
		TOY_ASSERT(0);
	}

	output_ibo->handle = buffer->handle;
	output_ibo->offset = primitive->first_index * primitive->index_stride;
	output_ibo->size = primitive->index_stride * primitive->index_count;
	output_ibo->padding = 0;
	output_ibo->source = buffer;
}


static void vertex_sub_buffer_alloc (
	toy_vulkan_mesh_primitive_asset_pool_t* vk_pool,
	VkDeviceSize size,
	uint32_t stride,
	toy_vulkan_mesh_primitive_t* vtx_output,
	toy_error_t* error)
{
	VkDeviceSize sub_buffer_offset;
	toy_vulkan_buffer_list_pool_t* buffer_pool = &vk_pool->vbo_pool;
	toy_vulkan_sub_buffer_t vbo;
	sub_buffer_offset = toy_vulkan_sub_buffer_list_alloc(buffer_pool, size, stride, &vbo);
	if (VK_WHOLE_SIZE != sub_buffer_offset) {
		TOY_ASSERT(0 == vbo.padding);
		vtx_output->vertex_stride = stride;
		vtx_output->vertex_padding = vbo.padding;
		vtx_output->vertex_count = size / stride;
		vtx_output->first_vertex = sub_buffer_offset / stride;
		toy_ok(error);
		return;
	}
	else {
		toy_err(TOY_ERROR_MEMORY_DEVICE_ALLOCATION_FAILED, "Alloc vertex buffer failed", error);
		// Todo: allocate a bigger buffer and copy data to that one
		return;
	}
}


static void vertex_sub_buffer_free (
	toy_vulkan_mesh_primitive_asset_pool_t* vk_pool,
	toy_vulkan_mesh_primitive_t* primitive)
{
	toy_vulkan_buffer_list_pool_t* buffer_pool = &vk_pool->vbo_pool;
	toy_vulkan_sub_buffer_t vbo;
	vbo.handle = buffer_pool->buffer.handle;
	vbo.offset = primitive->first_vertex * primitive->vertex_stride;
	vbo.size = primitive->vertex_stride * primitive->vertex_count;
	vbo.padding = primitive->vertex_padding;
	vbo.source = &buffer_pool->buffer;
	toy_vulkan_sub_buffer_list_free(buffer_pool, &vbo);
}


static void index_sub_buffer_alloc (
	toy_vulkan_mesh_primitive_asset_pool_t* vk_pool,
	VkDeviceSize size,
	uint8_t stride,
	toy_vulkan_mesh_primitive_t* idx_output,
	toy_error_t* error)
{
	TOY_ASSERT(size < UINT32_MAX);

	toy_vulkan_buffer_list_pool_t* buffer_pool = NULL;
	if (sizeof(uint16_t) == stride)
		buffer_pool = &vk_pool->ibo_pool16;
	else if (sizeof(uint8_t) == stride)
		buffer_pool = &vk_pool->ibo_pool8;
	else if (sizeof(uint32_t) == stride)
		buffer_pool = &vk_pool->ibo_pool32;
	else {
		TOY_ASSERT(0);
	}

	VkDeviceSize sub_buffer_offset;
	toy_vulkan_sub_buffer_t ibo;
	sub_buffer_offset = toy_vulkan_sub_buffer_list_alloc(buffer_pool, size, stride, &ibo);
	if (VK_WHOLE_SIZE != sub_buffer_offset) {
		TOY_ASSERT(0 == ibo.padding);
		idx_output->index_stride = stride;
		idx_output->index_count = size / stride;
		idx_output->first_index = sub_buffer_offset / stride;
		toy_ok(error);
		return;
	}
	else {
		toy_err(TOY_ERROR_MEMORY_DEVICE_ALLOCATION_FAILED, "Alloc index buffer failed", error);
		// Todo: allocate a bigger buffer and copy data to that one
		return;
	}
}


static void index_sub_buffer_free (
	toy_vulkan_mesh_primitive_asset_pool_t* vk_pool,
	toy_vulkan_mesh_primitive_t* primitive)
{
	toy_vulkan_buffer_list_pool_t* buffer_pool = NULL;
	if (sizeof(uint16_t) == primitive->index_stride)
		buffer_pool = &vk_pool->ibo_pool16;
	else if (sizeof(uint8_t) == primitive->index_stride)
		buffer_pool = &vk_pool->ibo_pool8;
	else if (sizeof(uint32_t) == primitive->index_stride)
		buffer_pool = &vk_pool->ibo_pool32;
	else {
		TOY_ASSERT(0);
	}

	toy_vulkan_sub_buffer_t ibo;
	ibo.handle = buffer_pool->buffer.handle;
	ibo.offset = primitive->first_index * primitive->index_stride;
	ibo.size = primitive->index_stride * primitive->index_count;
	ibo.padding = 0;
	ibo.source = &buffer_pool->buffer;
	toy_vulkan_sub_buffer_list_free(buffer_pool, &ibo);
}


void toy_alloc_vulkan_mesh_primitive (
	toy_vulkan_mesh_primitive_asset_pool_t* vk_pool,
	uint32_t vertex_attr_size,
	uint32_t vertex_count,
	uint32_t index_count,
	toy_vulkan_mesh_primitive_t* output,
	toy_error_t* error)
{
	vertex_sub_buffer_alloc(
		vk_pool,
		vertex_attr_size * vertex_count, vertex_attr_size,
		output, error);
	if (toy_is_failed(*error))
		return;

	// Default index type VK_INDEX_TYPE_UINT16
	uint8_t index_stride = index_count > UINT16_MAX ? sizeof(uint32_t) : sizeof(uint16_t);
	if (0 == index_count) {
		output->index_stride = index_stride;
		output->index_count = 0;
		output->first_index = 0;
	}
	else {
		index_sub_buffer_alloc(
			vk_pool,
			index_count * index_stride, index_stride,
			output, error);
		if (toy_unlikely(toy_is_failed(*error))) {
			vertex_sub_buffer_free(vk_pool, output);
			return;
		}
	}

	toy_ok(error);
}


void toy_free_vulkan_mesh_primitive (
	toy_vulkan_mesh_primitive_asset_pool_t* primitive_pool,
	toy_vulkan_mesh_primitive_t* primitive)
{
	vertex_sub_buffer_free(primitive_pool, primitive);
	if (primitive->index_count > 0)
		index_sub_buffer_free(primitive_pool, primitive);
}


void toy_vkcmd_copy_mesh_primitive_data (
	VkCommandBuffer cmd,
	toy_vulkan_mesh_primitive_asset_pool_t* vk_pool,
	toy_vulkan_sub_buffer_p vbo,
	toy_vulkan_sub_buffer_p ibo,
	toy_vulkan_mesh_primitive_t* dst)
{
	toy_vulkan_sub_buffer_t dst_vbo;
	toy_vulkan_look_up_vertex_sub_buffer(vk_pool, dst, &dst_vbo);

	VkBufferCopy region;
	region.srcOffset = vbo->offset;
	region.dstOffset = dst_vbo.offset;
	region.size = vbo->size;
	vkCmdCopyBuffer(cmd, vbo->handle, dst_vbo.handle, 1, &region);

	if (NULL != ibo && VK_NULL_HANDLE != ibo->handle) {
		toy_vulkan_sub_buffer_t dst_ibo;
		toy_vulkan_look_up_index_sub_buffer(vk_pool, dst, &dst_ibo);

		VkBufferCopy region;
		region.srcOffset = ibo->offset;
		region.dstOffset = dst_ibo.offset;
		region.size = ibo->size;
		vkCmdCopyBuffer(cmd, ibo->handle, dst_ibo.handle, 1, &region);
	}
}
