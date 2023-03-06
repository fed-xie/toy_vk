#include "../../include/platform/vulkan/toy_vulkan_buffer.h"

#include "../../toy_assert.h"

void toy_create_vulkan_buffer (
	VkDevice dev,
	VkBufferUsageFlags buffer_usage,
	const VkMemoryPropertyFlags* property_flags,
	uint32_t flag_count,
	VkDeviceSize size,
	const VkPhysicalDeviceMemoryProperties* mem_props,
	toy_vulkan_binding_allocator_t* binding_alc,
	const VkAllocationCallbacks* vk_alc_cb,
	toy_vulkan_buffer_t* output,
	toy_error_t* error)
{
	VkBufferCreateInfo buffer_ci;
	buffer_ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_ci.pNext = NULL;
	buffer_ci.flags = 0;
	buffer_ci.size = size;
	buffer_ci.usage = buffer_usage;
	buffer_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	buffer_ci.queueFamilyIndexCount = 0;
	buffer_ci.pQueueFamilyIndices = NULL;

	VkBuffer buffer_handle = VK_NULL_HANDLE;
	VkResult vk_err = vkCreateBuffer(dev, &buffer_ci, vk_alc_cb, &buffer_handle);
	if (toy_unlikely(VK_SUCCESS != vk_err)) {
		toy_err_vkerr(TOY_ERROR_CREATE_OBJECT_FAILED, vk_err, "vkCreateBuffer failed", error);
		return;
	}

	VkMemoryRequirements req;
	vkGetBufferMemoryRequirements(dev, buffer_handle, &req);

	toy_bind_vulkan_buffer_memory(
		dev, buffer_handle, &req,
		property_flags, flag_count,
		mem_props, binding_alc,
		&output->binding,
		error);
	if (toy_is_failed(*error)) {
		vkDestroyBuffer(dev, buffer_handle, vk_alc_cb);
		return;
	}

	output->handle = buffer_handle;
	output->size = size;
	output->alignment = req.alignment;

	toy_ok(error);
}


void toy_destroy_vulkan_buffer (
	VkDevice dev,
	toy_vulkan_buffer_t* buffer,
	const VkAllocationCallbacks* vk_alc_cb)
{
	vkDestroyBuffer(dev, buffer->handle, vk_alc_cb);
	toy_free_vulkan_memory_binding(&buffer->binding);
}


void toy_init_vulkan_buffer_stack (
	toy_vulkan_buffer_t* buffer,
	toy_vulkan_buffer_stack_t* output)
{
	output->next = NULL;
	if (&output->buffer != buffer)
		output->buffer = *buffer;
	output->top = 0;
}


// return offset of buffer, return VK_WHOLE_SIZE when failed
VkDeviceSize toy_vulkan_sub_buffer_alloc_L (
	toy_vulkan_buffer_stack_t* stack,
	VkDeviceSize alignment,
	VkDeviceSize size,
	toy_vulkan_sub_buffer_t* output)
{
	VkDeviceSize offset = stack->top;
	VkDeviceSize padding = 0;
	alignment = stack->buffer.alignment > alignment ? stack->buffer.alignment : alignment;
	if (toy_likely(alignment > 0)) {
		VkDeviceSize mask = alignment - 1;
		padding = (alignment - (offset & mask)) & mask; // padding % alignment == padding & mask
		offset += padding;
	}

	if (toy_unlikely(offset + size > stack->buffer.size))
		return VK_WHOLE_SIZE;

	stack->top += padding + size;

	output->handle = stack->buffer.handle;
	output->offset = offset;
	output->size = size;
	output->padding = padding;
	output->source = &stack->buffer;
	return offset;
}


void toy_vulkan_sub_buffer_free_L (
	toy_vulkan_buffer_stack_t* stack,
	toy_vulkan_sub_buffer_t* sub_buffer)
{
	TOY_ASSERT(stack->buffer.handle == sub_buffer->handle);
	TOY_ASSERT(sub_buffer->offset < stack->top);
	stack->top = sub_buffer->offset - sub_buffer->padding;
}


void toy_create_vulkan_buffer_list_pool (
	toy_vulkan_buffer_t* buffer,
	const toy_allocator_t* chunk_alc,
	toy_vulkan_buffer_list_pool_t* output,
	toy_error_t* error)
{
	toy_vulkan_buffer_list_chunk_t* chunk = toy_alloc(chunk_alc, sizeof(toy_vulkan_buffer_list_chunk_t));
	if (toy_unlikely(NULL == chunk)) {
		toy_err(TOY_ERROR_MEMORY_HOST_ALLOCATION_FAILED, "alloc vulkan buffer list chunk failed", error);
		return;
	}
	chunk->next = NULL;
	chunk->offset = 0;
	chunk->size = buffer->size;

	output->next = NULL;
	if (&output->buffer != buffer)
		output->buffer = *buffer;

	output->chunk_head = chunk;
	output->next_chunk = &output->chunk_head;
	output->chunk_alc = *chunk_alc;

	toy_ok(error);
}


// toy_vulkan_buffer_t should be destroyed by caller
void toy_destroy_vulkan_buffer_list_pool (
	toy_vulkan_buffer_list_pool_t* list_pool)
{
	TOY_ASSERT(NULL != list_pool->chunk_head && list_pool->buffer.size == list_pool->chunk_head->size);
	TOY_ASSERT(NULL == list_pool->chunk_head->next);

	toy_free(&list_pool->chunk_alc, list_pool->chunk_head);
}


static VkDeviceSize toy_select_next_chunk (
	toy_vulkan_buffer_list_pool_t* list_pool)
{
	list_pool->next_chunk = &list_pool->chunk_head;;
	if (NULL == list_pool->chunk_head)
		return 0;

	VkDeviceSize chunk_size = 0;
	toy_vulkan_buffer_list_chunk_p* biggest = &list_pool->chunk_head;
	toy_vulkan_buffer_list_chunk_p* next = &(list_pool->chunk_head->next);
	while (NULL != *next) {
		if ((*next)->size > (*biggest)->size) {
			biggest = next;
			chunk_size = (*next)->size;
		}
		next = &((*next)->next);
	}

	list_pool->next_chunk = biggest;
	return chunk_size;
}


static bool toy_vk_sub_buffer_list_alloc_impl (
	toy_vulkan_buffer_list_pool_t* list_pool,
	toy_vulkan_buffer_list_chunk_p* chunk_p,
	VkDeviceSize size,
	VkDeviceSize alignment,
	toy_vulkan_sub_buffer_t* output)
{
	toy_vulkan_buffer_list_chunk_p chunk = *chunk_p;
	VkDeviceSize start = chunk->offset;
	VkDeviceSize padding = 0;
	alignment = list_pool->buffer.alignment > alignment ? list_pool->buffer.alignment : alignment;
	if (alignment > 0) {
		const VkDeviceSize mask = alignment - 1;
		// assert alignment is 2^N
		TOY_ASSERT((alignment & mask) == 0);

		padding = (alignment - (start & mask)) & mask; // (alignment - (offset % alignment)) % alignment
	}
	VkDeviceSize end = start + padding + size;

	if (toy_unlikely(end > chunk->offset + chunk->size))
		return false;

	output->handle = list_pool->buffer.handle;
	output->offset = start + padding;
	output->size = size;
	output->padding = padding;
	output->source = &list_pool->buffer;

	// Chunk is bigger, break it
	if (end < chunk->offset + chunk->size) {
		chunk->size = chunk->offset + chunk->size - end;
		chunk->offset = end;
		return true;
	}

	// Just enough, remove this chunk from list_pool
	*chunk_p = chunk->next;
	if (*(list_pool->next_chunk) == chunk)
		toy_select_next_chunk(list_pool);
	toy_free(&list_pool->chunk_alc, chunk);
	return true;
}


// return offset of buffer, return VK_WHOLE_SIZE when failed
VkDeviceSize toy_vulkan_sub_buffer_list_alloc (
	toy_vulkan_buffer_list_pool_t* list_pool,
	VkDeviceSize size,
	VkDeviceSize alignment,
	toy_vulkan_sub_buffer_t* output)
{
	if (NULL == *(list_pool->next_chunk))
		return VK_WHOLE_SIZE;

	if (toy_vk_sub_buffer_list_alloc_impl(list_pool, list_pool->next_chunk, size, alignment, output))
		return output->offset;

	// Alloc failed from "next_chunk", try and select a new "next_chunk"
	if (0 == toy_select_next_chunk(list_pool))
		return VK_WHOLE_SIZE;
	if (toy_vk_sub_buffer_list_alloc_impl(list_pool, list_pool->next_chunk, size, alignment, output))
		return output->offset;

	return VK_WHOLE_SIZE;
}


void toy_vulkan_sub_buffer_list_free (
	toy_vulkan_buffer_list_pool_t* list_pool,
	toy_vulkan_sub_buffer_t* sub_buffer)
{
	TOY_ASSERT(NULL != list_pool && NULL != sub_buffer);
	TOY_ASSERT(list_pool->buffer.handle == sub_buffer->handle);

	VkDeviceSize start = sub_buffer->offset - sub_buffer->padding;
	VkDeviceSize end = sub_buffer->offset + sub_buffer->size;

	toy_vulkan_buffer_list_chunk_p chunk = list_pool->chunk_head;
	toy_vulkan_buffer_list_chunk_p* chunk_p = &list_pool->chunk_head;
	while (NULL != chunk) {
		if (chunk->offset > end) {
			toy_vulkan_buffer_list_chunk_p new_chunk = toy_alloc(&list_pool->chunk_alc, sizeof(toy_vulkan_buffer_list_chunk_t));
			TOY_ASSERT(NULL != new_chunk);
			new_chunk->next = chunk;
			new_chunk->offset = start;
			new_chunk->size = end - start;
			if (list_pool->next_chunk == &list_pool->chunk_head)
				list_pool->next_chunk = &new_chunk->next;
			list_pool->chunk_head = new_chunk;
			return;
		}
		if (chunk->offset == end) {
			chunk->size += end - start;
			chunk->offset = start;
			return;
		}
		if (chunk->offset + chunk->size == start) {
			chunk->size = end - chunk->offset;

			toy_vulkan_buffer_list_chunk_p next = chunk->next;
			if (NULL != next && next->offset == end) {
				chunk->size += next->size;
				chunk->next = next->next;
				if (*(list_pool->next_chunk) == next)
					list_pool->next_chunk = chunk_p;
				toy_free(&list_pool->chunk_alc, next);
			}
			return;
		}
		TOY_ASSERT(chunk->offset + chunk->size < start);
		chunk_p = &chunk->next;
		chunk = chunk->next;
	}

	toy_vulkan_buffer_list_chunk_p new_chunk = toy_alloc(&list_pool->chunk_alc, sizeof(toy_vulkan_buffer_list_chunk_t));
	TOY_ASSERT(NULL != new_chunk);
	new_chunk->next = NULL;
	new_chunk->offset = start;
	new_chunk->size = end - start;
	*chunk_p = new_chunk;
	return;
}
