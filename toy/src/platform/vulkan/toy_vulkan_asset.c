#include "../../include/platform/vulkan/toy_vulkan_asset.h"

#include "../../toy_assert.h"
#include "../../include/toy_log.h"

static toy_vulkan_buffer_list_pool_t* toy_alloc_vulkan_buffer_list_pool (
	VkBufferUsageFlags usage,
	VkDeviceSize size,
	toy_vulkan_memory_allocator_p vk_allocator,
	toy_error_t* error)
{
	toy_vulkan_buffer_list_pool_t* ret = (toy_vulkan_buffer_list_pool_t*)toy_alloc_aligned(
		&vk_allocator->mem_alc->buddy_alc, sizeof(toy_vulkan_buffer_list_pool_t), sizeof(void*));
	if (toy_unlikely(NULL == ret)) {
		toy_err(TOY_ERROR_MEMORY_HOST_ALLOCATION_FAILED, "Failed to malloc VBO pool", error);
		return NULL;
	}

	toy_create_vulkan_buffer(
		vk_allocator->device,
		usage, size,
		&vk_allocator->memory_properties,
		&vk_allocator->vk_list_alc,
		vk_allocator->vk_alc_cb_p,
		&ret->buffer,
		error);
	if (toy_unlikely(toy_is_failed(*error)))
		goto FAIL_CREATE_BUFFER;

	TOY_ASSERT(sizeof(toy_vulkan_buffer_list_chunk_t) == sizeof(toy_vulkan_memory_list_chunk_t));
	toy_create_vulkan_buffer_list_pool(&ret->buffer, &vk_allocator->chunk_alc, ret, error);
	if (toy_unlikely(toy_is_failed(*error)))
		goto FAIL_CREATE_BUFFER_POOL;

	toy_ok(error);
	return ret;

FAIL_CREATE_BUFFER_POOL:
	vkDestroyBuffer(vk_allocator->device, ret->buffer.handle, vk_allocator->vk_alc_cb_p);
FAIL_CREATE_BUFFER:
	toy_free_aligned(&vk_allocator->mem_alc->buddy_alc, ret);
	return NULL;
}


static void toy_free_vulkan_buffer_list_pool (
	toy_vulkan_memory_allocator_p vk_allocator,
	const VkAllocationCallbacks* vk_buffer_alc_cb,
	toy_vulkan_buffer_list_pool_t* pool)
{
	vkDestroyBuffer(vk_allocator->device, pool->buffer.handle, vk_buffer_alc_cb);
	toy_destroy_vulkan_buffer_list_pool(pool);
	toy_free_aligned(&vk_allocator->mem_alc->buddy_alc, pool);
}


static void toy_free_vulkan_mesh_primitive_sub_buffer (
	toy_vulkan_buffer_list_pool_t* buffer_pool,
	toy_vulkan_sub_buffer_t* sub_buffer)
{
	TOY_ASSERT(NULL != buffer_pool);
	do {
		if (sub_buffer->source == &buffer_pool->buffer) {
			toy_vulkan_sub_buffer_list_free(buffer_pool, sub_buffer);
			return;
		}
		buffer_pool = buffer_pool->next;
	} while (NULL != buffer_pool);
	TOY_ASSERT(0);
}


void toy_create_vulkan_mesh_primitive_asset_pool (
	VkDeviceSize vertex_buffer_size,
	VkDeviceSize index_buffer_size,
	toy_vulkan_memory_allocator_p vk_allocator,
	toy_vulkan_mesh_primitive_asset_pool_t* output,
	toy_error_t* error)
{
	output->vbo_pools = toy_alloc_vulkan_buffer_list_pool(
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		vertex_buffer_size,
		vk_allocator,
		error);
	if (toy_unlikely(NULL == output->vbo_pools))
		goto FAIL_VBO;

	output->ibo_pools = toy_alloc_vulkan_buffer_list_pool(
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		index_buffer_size,
		vk_allocator,
		error);
	if (toy_unlikely(NULL == output->ibo_pools))
		goto FAIL_IBO;

	output->list_pool_alc = &vk_allocator->mem_alc->buddy_alc;
	output->vk_allocator = vk_allocator;

	toy_ok(error);
	return;

FAIL_IBO:
	toy_free_vulkan_buffer_list_pool(vk_allocator, vk_allocator->vk_alc_cb_p, output->vbo_pools);
FAIL_VBO:
	toy_log_error(error);
	return;
}


void toy_destroy_vulkan_mesh_primitive_asset_pool (
	toy_vulkan_mesh_primitive_asset_pool_t* vk_pool)
{
	toy_vulkan_buffer_list_pool_t* pool = vk_pool->ibo_pools;
	while (NULL != pool) {
		toy_vulkan_buffer_list_pool_t* next = pool->next;
		toy_free_vulkan_buffer_list_pool(vk_pool->vk_allocator, vk_pool->vk_allocator->vk_alc_cb_p, pool);
		pool = next;
	}

	pool = vk_pool->vbo_pools;
	while (NULL != pool) {
		toy_vulkan_buffer_list_pool_t* next = pool->next;
		toy_free_vulkan_buffer_list_pool(vk_pool->vk_allocator, vk_pool->vk_allocator->vk_alc_cb_p, pool);
		pool = next;
	}
}


static void toy_vulkan_sub_buffer_lists_alloc (
	toy_vulkan_buffer_list_pool_t** buffer_pool_head,
	VkDeviceSize size,
	VkDeviceSize alignment,
	VkBufferUsageFlags usage,
	toy_vulkan_memory_allocator_p vk_allocator,
	toy_vulkan_sub_buffer_t* output,
	toy_error_t* error)
{
	TOY_ASSERT(NULL != buffer_pool_head && NULL != *buffer_pool_head);

	VkDeviceSize sub_buffer_size;
	toy_vulkan_buffer_list_pool_t* buffer_pool = *buffer_pool_head;
	while (NULL != buffer_pool) {
		sub_buffer_size = toy_vulkan_sub_buffer_list_alloc(buffer_pool, size, alignment, output);
		if (VK_WHOLE_SIZE != sub_buffer_size) {
			toy_ok(error);
			return;
		}
		buffer_pool = buffer_pool->next;
	}

	VkDeviceSize buffer_size = (*buffer_pool_head)->buffer.size;
	if (size > buffer_size)
		buffer_size = size;
	buffer_pool = toy_alloc_vulkan_buffer_list_pool(
		usage, buffer_size,
		vk_allocator,
		error);
	if (toy_unlikely(NULL == buffer_pool))
		return;

	buffer_pool->next = *buffer_pool_head;
	*buffer_pool_head = buffer_pool;

	sub_buffer_size = toy_vulkan_sub_buffer_list_alloc(buffer_pool, size, alignment, output);
	TOY_ASSERT(VK_WHOLE_SIZE != sub_buffer_size && 0 != sub_buffer_size);
	toy_ok(error);
}


static void toy_vulkan_sub_buffer_lists_free (
	toy_vulkan_buffer_list_pool_t** list_head,
	toy_vulkan_sub_buffer_p sub_buffer)
{
	toy_vulkan_buffer_list_pool_t* buffer_pool = *list_head;
	while (NULL != buffer_pool) {
		if (sub_buffer->source == &buffer_pool->buffer)
			toy_vulkan_sub_buffer_list_free(buffer_pool, sub_buffer);
		
		buffer_pool = buffer_pool->next;
	}
	TOY_ASSERT(0);
}



void toy_alloc_vulkan_mesh_primitive (
	toy_vulkan_mesh_primitive_asset_pool_t* vk_pool,
	VkDeviceSize vertex_size,
	VkDeviceSize index_size,
	toy_vulkan_mesh_primitive_t* output,
	toy_error_t* error)
{
	toy_vulkan_sub_buffer_lists_alloc(
		&vk_pool->vbo_pools,
		vertex_size, 0,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		vk_pool->vk_allocator,
		&output->vbo,
		error);
	if (toy_is_failed(*error))
		return;

	if (0 == index_size) {
		output->ibo.handle = VK_NULL_HANDLE;
		output->ibo.offset = 0;
		output->ibo.size = 0;
		output->ibo.source = NULL;
	}
	else {
		toy_vulkan_sub_buffer_lists_alloc(
			&vk_pool->ibo_pools,
			index_size, 0,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			vk_pool->vk_allocator,
			&output->ibo,
			error);
		if (toy_unlikely(toy_is_failed(*error))) {
			toy_vulkan_sub_buffer_lists_free(&vk_pool->vbo_pools, &output->vbo);
			return;
		}
	}

	output->vertex_count = 0;
	output->index_count = 0;
	output->index_type = VK_INDEX_TYPE_NONE_KHR;

	toy_ok(error);
}


void toy_free_vulkan_mesh_primitive (
	toy_vulkan_mesh_primitive_asset_pool_t* primitive_pool,
	toy_vulkan_mesh_primitive_t* primitive)
{
	toy_free_vulkan_mesh_primitive_sub_buffer(primitive_pool->vbo_pools, &primitive->vbo);
	if (VK_NULL_HANDLE != primitive->ibo.handle)
		toy_free_vulkan_mesh_primitive_sub_buffer(primitive_pool->ibo_pools, &primitive->ibo);
}
