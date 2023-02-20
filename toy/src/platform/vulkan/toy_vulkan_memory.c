#include "../../include/platform/vulkan/toy_vulkan_memory.h"

#include "../../toy_assert.h"

static uint32_t toy_select_vulkan_memory_type_index (
	const VkMemoryRequirements* req,
	VkMemoryPropertyFlags suggest_flag,
	const VkPhysicalDeviceMemoryProperties* phy_dev_mem_props)
{
	uint32_t index = UINT32_MAX;

	for (uint32_t i = 0; i < phy_dev_mem_props->memoryTypeCount; ++i) {
		if (req->memoryTypeBits & (1 << i)) { // memory type i supported
			if (suggest_flag == (suggest_flag & phy_dev_mem_props->memoryTypes[i].propertyFlags)) {
				// memory type flag fits
				index = i;
				// Todo: track heap size and select an available heap
				uint32_t heap_index = phy_dev_mem_props->memoryTypes[i].heapIndex;
				phy_dev_mem_props->memoryHeaps[heap_index].size;
				break;
			}
		}
	}
	return index;
}


void toy_bind_vulkan_image_memory (
	VkDevice dev,
	VkImage image,
	const VkMemoryPropertyFlags* property_flags,
	uint32_t flag_count,
	const VkPhysicalDeviceMemoryProperties* mem_props,
	toy_vulkan_binding_allocator_t* binding_alc,
	toy_vulkan_memory_binding_t* output,
	toy_error_t* error)
{
	VkMemoryRequirements req;
	vkGetImageMemoryRequirements(dev, image, &req);

	for (uint32_t fi = 0; fi < flag_count; ++fi) {
		uint32_t type_index = toy_select_vulkan_memory_type_index(&req, property_flags[fi], mem_props);
		if (VK_MAX_MEMORY_TYPES <= type_index)
			continue;

		binding_alc->alloc(binding_alc->ctx, type_index, &req, output, error);
		if (toy_unlikely(toy_is_failed(*error))) {
			toy_err(TOY_ERROR_MEMORY_DEVICE_ALLOCATION_FAILED, "Alloc image memory failed", error);
			return;
		}

		VkResult vk_err = vkBindImageMemory(dev, image, output->memory, output->offset);
		if (VK_SUCCESS != vk_err) {
			toy_free_vulkan_memory_binding(output);
			toy_err_vkerr(TOY_ERROR_OPERATION_FAILED, vk_err, "vkBindImageMemory failed", error);
			return;
		}

		toy_ok(error);
		return;
	}

	toy_err(TOY_ERROR_ASSERT_FAILED, "Can't find suitable image memory type index", error);
}


void toy_bind_vulkan_buffer_memory (
	VkDevice dev,
	VkBuffer buffer,
	const VkMemoryRequirements* req,
	const VkMemoryPropertyFlags* property_flags,
	uint32_t flag_count,
	const VkPhysicalDeviceMemoryProperties* mem_props,
	toy_vulkan_binding_allocator_t* binding_alc,
	toy_vulkan_memory_binding_t* output,
	toy_error_t* error)
{
	for (uint32_t fi = 0; fi < flag_count; ++fi) {
		uint32_t type_index = toy_select_vulkan_memory_type_index(req, property_flags[fi], mem_props);
		if (VK_MAX_MEMORY_TYPES <= type_index)
			continue;

		binding_alc->alloc(binding_alc->ctx, type_index, req, output, error);
		if (toy_unlikely(toy_is_failed(*error))) {
			toy_err(TOY_ERROR_MEMORY_DEVICE_ALLOCATION_FAILED, "Alloc buffer memory failed", error);
			return;
		}

		VkResult vk_err = vkBindBufferMemory(dev, buffer, output->memory, output->offset);
		if (VK_SUCCESS != vk_err) {
			toy_free_vulkan_memory_binding(output);
			toy_err_vkerr(TOY_ERROR_OPERATION_FAILED, vk_err, "vkBindBufferMemory failed", error);
			return;
		}

		toy_ok(error);
		return;
	}

	toy_err(TOY_ERROR_ASSERT_FAILED, "Can't find suitable buffer memory type index", error);
}


void toy_init_vulkan_memory_stack (
	VkDeviceMemory memory,
	VkDeviceSize memory_offset,
	VkDeviceSize size,
	uint32_t type_index,
	VkMemoryPropertyFlags property_flags,
	toy_vulkan_memory_stack_t* output)
{
	output->next = NULL;
	output->memory = memory;
	output->bottom = memory_offset;
	output->size = size;
	output->type_index = type_index;
	output->property_flags = property_flags;
	output->left_top = memory_offset;
	output->right_top = memory_offset + size;
}


VkResult toy_alloc_vulkan_memory_stack (
	VkDevice dev,
	VkDeviceSize size,
	uint32_t type_index,
	const VkPhysicalDeviceMemoryProperties* phy_dev_mem_props,
	const VkAllocationCallbacks* vk_alc_cb,
	toy_vulkan_memory_stack_t* output)
{
	VkDeviceMemory vk_memory = VK_NULL_HANDLE;
	VkMemoryAllocateInfo mem_ai;
	mem_ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	mem_ai.pNext = NULL;
	mem_ai.allocationSize = size;
	mem_ai.memoryTypeIndex = type_index;
	VkResult vk_err = vkAllocateMemory(dev, &mem_ai, vk_alc_cb, &vk_memory);
	if (toy_unlikely(VK_SUCCESS != vk_err))
		return vk_err;

	toy_init_vulkan_memory_stack(
		vk_memory, 0, size,
		type_index, phy_dev_mem_props->memoryTypes[type_index].propertyFlags,
		output);
	return VK_SUCCESS;
}


void toy_vulkan_stack_free_L (
	toy_vulkan_memory_stack_p stack,
	toy_vulkan_memory_binding_t* binding)
{
	TOY_ASSERT(NULL != stack && NULL != binding);
	TOY_ASSERT(stack->memory == binding->memory);

	const VkDeviceSize offset = binding->offset;
	const VkDeviceSize size = binding->size;
	const VkDeviceSize padding = binding->padding;

	TOY_ASSERT(stack->left_top == offset + size);
	stack->left_top = offset - padding;
	TOY_ASSERT(stack->left_top >= stack->bottom);
}


void toy_vulkan_stack_alloc_L (
	toy_vulkan_memory_stack_p stack,
	VkDeviceSize size,
	VkDeviceSize alignment,
	toy_vulkan_memory_binding_t* output,
	toy_error_t* error)
{
	TOY_ASSERT(NULL != stack);
	if (toy_unlikely(0 == size || size > stack->size)) {
		toy_err(TOY_ERROR_MEMORY_HOST_ALLOCATION_FAILED, "No enough space in vulkan stack", error);
		return;
	}

	VkDeviceSize offset = stack->left_top;
	VkDeviceSize padding = 0;
	if (alignment > 0) {
		const VkDeviceSize mask = alignment - 1;
		// assert alignment is 2^N
		TOY_ASSERT((alignment & mask) == 0);

		padding = (alignment - (offset & mask)) & mask; // (alignment - (offset % alignment)) % alignment
		offset += padding;
	}

	if (offset + size > stack->right_top) {
		toy_err(TOY_ERROR_MEMORY_HOST_ALLOCATION_FAILED, "No enough space in vulkan stack", error);
		return;
	}

	output->memory = stack->memory;
	output->offset = offset;
	output->size = size;
	output->property_flags = stack->property_flags;
	output->source = stack;
	output->free = (toy_vulkan_free_fp)toy_vulkan_stack_free_L;
	output->padding = padding;

	stack->left_top = offset + size;
	toy_ok(error);
}


void toy_vulkan_stack_free_R (
	toy_vulkan_memory_stack_p stack,
	toy_vulkan_memory_binding_t* binding)
{
	TOY_ASSERT(NULL != stack && NULL != binding);
	TOY_ASSERT(stack->memory == binding->memory);

	const VkDeviceSize offset = binding->offset;
	const VkDeviceSize padding = binding->padding;
	const VkDeviceSize size = binding->size;

	TOY_ASSERT(stack->right_top == offset);
	stack->right_top = offset + size + padding;
	TOY_ASSERT(stack->right_top <= stack->bottom + stack->size);
}


void toy_vulkan_stack_alloc_R (
	toy_vulkan_memory_stack_p stack,
	VkDeviceSize size,
	VkDeviceSize alignment,
	toy_vulkan_memory_binding_t* output,
	toy_error_t* error)
{
	TOY_ASSERT(NULL != stack);
	if (toy_unlikely(0 == size || size > stack->size)) {
		toy_err(TOY_ERROR_MEMORY_HOST_ALLOCATION_FAILED, "No enough space in vulkan stack", error);
		return;
	}

	if (size > stack->right_top || stack->right_top - size <= stack->left_top) {
		toy_err(TOY_ERROR_MEMORY_HOST_ALLOCATION_FAILED, "No enough space in vulkan stack", error);
		return;
	}

	VkDeviceSize offset = stack->right_top - size;
	VkDeviceSize padding = 0;
	if (alignment > 0) {
		const VkDeviceSize mask = alignment - 1;
		// assert alignment is 2^N
		TOY_ASSERT((alignment & mask) == 0);
		padding = offset - (offset & (~mask)); // offset & (~mask) == offset - (offset % alignment)
		offset = offset - padding;
	}

	output->memory = stack->memory;
	output->offset = offset;
	output->size = size;
	output->property_flags = stack->property_flags;
	output->source = stack;
	output->free = (toy_vulkan_free_fp)toy_vulkan_stack_free_R;
	output->padding = padding;

	stack->right_top = offset;

	toy_ok(error);
}


void toy_alloc_vulkan_memory_pool (
	VkDevice dev,
	VkDeviceSize block_size,
	uint16_t block_count,
	uint32_t type_index,
	const VkPhysicalDeviceMemoryProperties* phy_dev_mem_props,
	const toy_allocator_t* alc,
	const VkAllocationCallbacks* vk_alc_cb,
	toy_vulkan_memory_pool_p* output,
	toy_error_t* error)
{
	TOY_ASSERT(NULL != error);

	VkDeviceMemory vk_memory = VK_NULL_HANDLE;
	VkMemoryAllocateInfo mem_ai;
	mem_ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	mem_ai.pNext = NULL;
	mem_ai.allocationSize = block_size * block_count;
	mem_ai.memoryTypeIndex = type_index;
	VkResult vk_err = vkAllocateMemory(dev, &mem_ai, vk_alc_cb, &vk_memory);
	if (toy_unlikely(VK_SUCCESS != vk_err)) {
		toy_err_vkerr(TOY_ERROR_MEMORY_DEVICE_ALLOCATION_FAILED, vk_err, "vkAllocateMemory failed", error);
		return;
	}

	size_t host_pool_size = sizeof(toy_vulkan_memory_pool_t) + block_count * sizeof(uint16_t);
	toy_vulkan_memory_pool_p pool = (toy_vulkan_memory_pool_p)toy_alloc_aligned(alc, host_pool_size, sizeof(void*));
	if (toy_unlikely(NULL == pool)) {
		vkFreeMemory(dev, vk_memory, vk_alc_cb);
		toy_err(TOY_ERROR_MEMORY_HOST_ALLOCATION_FAILED, "Malloc pool structure failed", error);
		return;
	}

	pool->next = NULL;
	pool->memory = vk_memory;
	pool->bottom = 0;
	pool->size = block_size * block_count;
	pool->type_index = type_index;
	pool->property_flags = phy_dev_mem_props->memoryTypes[type_index].propertyFlags;
	pool->block_size = block_size;
	pool->block_count = block_count;
	pool->next_free_block_index = 0;
	pool->free_block_count = block_count;
	pool->indices_area = (uint16_t*)(((uintptr_t)pool) + sizeof(toy_vulkan_memory_pool_t));

	for (uint16_t i = 0; i < block_count; ++i)
		pool->indices_area[i] = i + 1;

	*output = pool;
	toy_ok(error);
}


void toy_free_vulkan_memory_pool (
	VkDevice dev,
	const toy_allocator_t* alc,
	const VkAllocationCallbacks* vk_alc_cb,
	toy_vulkan_memory_pool_p pool)
{
	vkFreeMemory(dev, pool->memory, vk_alc_cb);
	toy_free_aligned(alc, pool);
}


void toy_vulkan_pool_block_free (
	toy_vulkan_memory_pool_p pool,
	toy_vulkan_memory_binding_t* binding)
{
	TOY_ASSERT(NULL != pool && NULL != binding);
	TOY_ASSERT(pool->memory == binding->memory);
	TOY_ASSERT(pool->free_block_count < pool->block_count);
	TOY_ASSERT(pool->bottom <= binding->offset);
	TOY_ASSERT(binding->offset < (pool->bottom + pool->block_count * pool->block_size));
	TOY_ASSERT(((binding->offset - binding->padding) % pool->block_size) == 0);

	uint16_t block_index = (uint16_t)((binding->offset - binding->padding) / pool->block_size);
#if TOY_DEBUG
	TOY_ASSERT(pool->block_count == pool->indices_area[block_index]);
#endif
	pool->indices_area[block_index] = pool->next_free_block_index;
	pool->next_free_block_index = block_index;
	++(pool->free_block_count);
}


void toy_vulkan_pool_block_alloc (
	toy_vulkan_memory_pool_p pool,
	VkDeviceSize size,
	VkDeviceSize alignment,
	toy_vulkan_memory_binding_t* output,
	toy_error_t* error)
{
	TOY_ASSERT(NULL != pool && 0 != size && NULL != output);
	TOY_ASSERT(NULL != error);

	if (toy_unlikely(0 == pool->free_block_count)) {
		toy_err(TOY_ERROR_MEMORY_DEVICE_ALLOCATION_FAILED, "Out of pool memory, try malloc new one", error);
		return;
	}

	if (toy_unlikely(size > pool->block_size)) {
		toy_err(TOY_ERROR_MEMORY_DEVICE_ALLOCATION_FAILED, "Size too big, try pool with bigger block size", error);
		return;
	}

	if (toy_unlikely(alignment > 0 && ((pool->block_size % alignment) > 0))) {
		toy_err(TOY_ERROR_MEMORY_ALIGNMENT_ERROR, "Pool block size can not divided by alignment", error);
		return;
	}

	uint16_t block_index = pool->next_free_block_index;
	VkDeviceSize offset = pool->bottom + block_index * pool->block_size;

	pool->next_free_block_index = pool->indices_area[block_index];
	--(pool->free_block_count);
#if TOY_DEBUG
	pool->indices_area[block_index] = pool->block_count; // Set to check duplicated allocation
#endif

	output->memory = pool->memory;
	output->offset = offset;
	output->size = size;
	output->property_flags = pool->property_flags;
	output->source = pool;
	output->free = (toy_vulkan_free_fp)toy_vulkan_pool_block_free;
	output->padding = 0;

	toy_ok(error);
}


void toy_create_vulkan_memory_list (
	VkDevice dev,
	VkDeviceSize size,
	uint32_t type_index,
	const VkPhysicalDeviceMemoryProperties* phy_dev_mem_props,
	const VkAllocationCallbacks* vk_alloc_cb,
	const toy_allocator_t* chunk_alc, // allocator for toy_vulkan_memory_list_chunk_t
	toy_vulkan_memory_list_t* output,
	toy_error_t* error)
{
	TOY_ASSERT(NULL != error);
	TOY_ASSERT(NULL != chunk_alc);
	TOY_ASSERT(0 == (size & (size - 1))); // assert size is 2^N

	VkDeviceMemory vk_memory = VK_NULL_HANDLE;
	VkMemoryAllocateInfo mem_ai;
	mem_ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	mem_ai.pNext = NULL;
	mem_ai.allocationSize = size;
	mem_ai.memoryTypeIndex = type_index;
	VkResult vk_err = vkAllocateMemory(dev, &mem_ai, vk_alloc_cb, &vk_memory);
	if (toy_unlikely(VK_SUCCESS != vk_err)) {
		toy_err_vkerr(TOY_ERROR_MEMORY_DEVICE_ALLOCATION_FAILED, vk_err, "vkAllocateMemory failed", error);
		return;
	}

	output->next = NULL;
	output->memory = vk_memory;
	output->bottom = 0;
	output->size = size;
	output->type_index = type_index;
	output->property_flags = phy_dev_mem_props->memoryTypes[type_index].propertyFlags;

	toy_vulkan_memory_list_chunk_t* chunk = toy_alloc(chunk_alc, sizeof(toy_vulkan_memory_list_chunk_t));
	if (toy_unlikely(NULL == chunk)) {
		toy_err_vkerr(TOY_ERROR_MEMORY_HOST_ALLOCATION_FAILED, vk_err, "malloc vulkan memory list chunk failed", error);
		vkFreeMemory(dev, vk_memory, vk_alloc_cb);
		return;
	}
	chunk->next = NULL;
	chunk->offset = 0;
	chunk->size = size;

	output->chunk_head = chunk;
	output->next_chunk = &(output->chunk_head);
	output->chunk_alc = *chunk_alc;

	toy_ok(error);
}


void toy_destroy_vulkan_memory_list (
	VkDevice dev,
	toy_vulkan_memory_list_t* list,
	const VkAllocationCallbacks* vk_alc_cb)
{
	TOY_ASSERT(NULL != list->chunk_head && list->size == list->chunk_head->size);
	TOY_ASSERT(NULL == list->chunk_head->next);

	toy_free(&list->chunk_alc, list->chunk_head);
	vkFreeMemory(dev, list->memory, vk_alc_cb);
}


static VkDeviceSize toy_select_next_chunk (
	toy_vulkan_memory_list_p list)
{
	list->next_chunk = &list->chunk_head;
	if (NULL == list->chunk_head)
		return 0;

	VkDeviceSize chunk_size = 0;
	toy_vulkan_memory_list_chunk_p* biggest = &list->chunk_head;
	toy_vulkan_memory_list_chunk_p* next = &(list->chunk_head->next);
	while (NULL != *next) {
		if ((*next)->size > (*biggest)->size) {
			biggest = next;
			chunk_size = (*next)->size;
		}
		next = &((*next)->next);
	}

	list->next_chunk = biggest;
	return chunk_size;
}


void toy_vulkan_memory_chunk_free (
	toy_vulkan_memory_list_p list,
	toy_vulkan_memory_binding_t* binding)
{
	TOY_ASSERT(NULL != list && NULL != binding);
	TOY_ASSERT(binding->offset >= binding->padding);

	VkDeviceSize start = binding->offset - binding->padding;
	VkDeviceSize end = binding->offset + binding->size;

	toy_vulkan_memory_list_chunk_p prev = NULL;
	toy_vulkan_memory_list_chunk_p chunk = list->chunk_head;
	while (NULL != chunk) {
		if (chunk->offset > end) {
			toy_vulkan_memory_list_chunk_p new_chunk = toy_alloc(&list->chunk_alc, sizeof(toy_vulkan_memory_list_chunk_t));
			TOY_ASSERT(NULL != new_chunk);
			new_chunk->next = chunk;
			new_chunk->offset = start;
			new_chunk->size = end - start;
			if (list->chunk_head == chunk)
				list->chunk_head = new_chunk;
			return;
		}
		if (chunk->offset == end) {
			chunk->size += end - start;
			chunk->offset = start;
			return;
		}
		if (chunk->offset + chunk->size == start) {
			chunk->offset = start;
			chunk->size = end - start;

			toy_vulkan_memory_list_chunk_p next = chunk->next;
			if (NULL != next && next->offset == end) {
				chunk->size += next->size;
				chunk->next = next->next;
				if (*(list->next_chunk) == next) {
					toy_vulkan_memory_list_chunk_p* next_chunk = &(list->chunk_head);
					while (chunk != *next_chunk)
						next_chunk = &((*next_chunk)->next);
					list->next_chunk = next_chunk;
				}
				toy_free(&list->chunk_alc, next);
			}
			return;
		}
		TOY_ASSERT(chunk->offset + chunk->size < start);
		prev = chunk;
		chunk = chunk->next;
	}

	toy_vulkan_memory_list_chunk_p new_chunk = toy_alloc(&list->chunk_alc, sizeof(toy_vulkan_memory_list_chunk_t));
	TOY_ASSERT(NULL != new_chunk);
	new_chunk->next = NULL;
	new_chunk->offset = start;
	new_chunk->size = end - start;
	if (NULL != prev)
		prev->next = new_chunk;
	else {
		list->chunk_head = new_chunk;
		list->next_chunk = &(list->chunk_head);
	}
	return;
}


static bool toy_vk_memlist_chunk_alloc_impl (
	toy_vulkan_memory_list_p list,
	toy_vulkan_memory_list_chunk_p* chunk_p,
	VkDeviceSize size,
	VkDeviceSize alignment,
	toy_vulkan_memory_binding_t* output)
{
	toy_vulkan_memory_list_chunk_p chunk = *chunk_p;
	VkDeviceSize offset = chunk->offset;
	VkDeviceSize padding = 0;
	if (alignment > 0) {
		const VkDeviceSize mask = alignment - 1;
		// assert alignment is 2^N
		TOY_ASSERT((alignment & mask) == 0);

		padding = (alignment - (offset & mask)) & mask; // (alignment - (offset % alignment)) % alignment
		offset += padding;
	}

	if (offset + size <= chunk->offset + chunk->size) {
		output->memory = list->memory;
		output->offset = offset;
		output->size = size;
		output->property_flags = list->property_flags;
		output->source = list;
		output->free = toy_vulkan_memory_chunk_free;
		output->padding = padding;

		// Chunk is bigger, break it
		if (offset + size < chunk->offset + chunk->size) {
			chunk->size = (chunk->offset + chunk->size) - (offset + size);
			chunk->offset = offset + size;
			return true;
		}

		// Just enough, remove this chunk from tree
		if (*chunk_p == chunk)
			*chunk_p = chunk->next;
		if (*(list->next_chunk) == chunk)
			toy_select_next_chunk(list);
		toy_free(&list->chunk_alc, chunk);
		return true;
	}

	return false;
}


void toy_vulkan_memory_list_chunk_alloc (
	toy_vulkan_memory_list_p list,
	VkDeviceSize size,
	VkDeviceSize alignment,
	toy_vulkan_memory_binding_t* output,
	toy_error_t* error)
{
	if (NULL == *(list->next_chunk)) {
		toy_err(TOY_ERROR_MEMORY_DEVICE_ALLOCATION_FAILED, "Out of Vulkan memory list", error);
		return;
	}

	if (toy_vk_memlist_chunk_alloc_impl(list, list->next_chunk, size, alignment, output)) {
		toy_ok(error);
		return;
	}

	if (0 == toy_select_next_chunk(list)) {
		toy_err(TOY_ERROR_MEMORY_DEVICE_ALLOCATION_FAILED, "Out of Vulkan memory list", error);
		return;
	}
	if (toy_vk_memlist_chunk_alloc_impl(list, list->next_chunk, size, alignment, output)) {
		toy_ok(error);
		return;
	}

	toy_err(TOY_ERROR_MEMORY_DEVICE_ALLOCATION_FAILED, "Out of Vulkan memory list", error);
}


static void toy_alloc_vulkan_binding_memory_via_lists (
	toy_vulkan_memory_allocator_t* alc,
	uint32_t type_index,
	const VkMemoryRequirements* req,
	toy_vulkan_memory_binding_t* output,
	toy_error_t* error)
{
	TOY_ASSERT(NULL != req && req->size > 0);
	toy_vulkan_memory_list_p list = alc->vk_mem_list[type_index];
	while (NULL != list) {
		toy_vulkan_memory_list_chunk_alloc(list, req->size, req->alignment, output, error);
		if (toy_is_ok(*error))
			return;

		list = list->next;
	}

	VkDeviceSize list_size = 32 * 1024 * 1024;
	VkDeviceSize heap_size = alc->memory_properties.memoryHeaps[alc->memory_properties.memoryTypes[type_index].heapIndex].size;
	if (list_size < req->size)
		list_size = UINT64_C(1) << toy_fls(req->size);
	if (list_size > heap_size)
		list_size = heap_size;
	if (list_size < req->size) {
		toy_err(TOY_ERROR_MEMORY_DEVICE_ALLOCATION_FAILED, "Too big memory size for this type_index", error);
		return;
	}

	toy_allocator_t* mem_alc = &alc->mem_alc->buddy_alc;
	toy_vulkan_memory_list_p new_list = (toy_vulkan_memory_list_p)toy_alloc_aligned(mem_alc, sizeof(toy_vulkan_memory_list_t), sizeof(void*));
	if (toy_unlikely(NULL == new_list)) {
		toy_err(TOY_ERROR_MEMORY_HOST_ALLOCATION_FAILED, "failed to create toy_vulkan_memory_list_t", error);
		return;
	}

	toy_create_vulkan_memory_list(alc->device, list_size, type_index, &alc->memory_properties, alc->vk_alc_cb_p, &alc->chunk_alc, new_list, error);
	if (toy_unlikely(toy_is_failed(*error))) {
		toy_free_aligned(mem_alc, new_list);
		return;
	}

	new_list->next = alc->vk_mem_list[type_index];
	alc->vk_mem_list[type_index] = new_list;

	toy_vulkan_memory_list_chunk_alloc(new_list, req->size, req->alignment, output, error);
}


static void toy_free_vulkan_memory (
	toy_vulkan_memory_allocator_t* vk_allocator,
	toy_vulkan_memory_binding_t* binding)
{
	vkFreeMemory(vk_allocator->device, binding->memory, vk_allocator->vk_alc_cb_p);
}


static void toy_alloc_vulkan_memory (
	toy_vulkan_memory_allocator_t* vk_allocator,
	uint32_t type_index,
	const VkMemoryRequirements* req,
	toy_vulkan_memory_binding_t* output,
	toy_error_t* error)
{
	VkDeviceMemory vk_memory = VK_NULL_HANDLE;

	VkMemoryAllocateInfo mem_ai;
	mem_ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	mem_ai.pNext = NULL;
	mem_ai.allocationSize = req->size;
	mem_ai.memoryTypeIndex = type_index;
	VkResult vk_err = vkAllocateMemory(vk_allocator->device, &mem_ai, vk_allocator->vk_alc_cb_p, &vk_memory);
	if (toy_unlikely(VK_SUCCESS != vk_err)) {
		toy_err_vkerr(TOY_ERROR_MEMORY_DEVICE_ALLOCATION_FAILED, vk_err, "vkAllocateMemory failed", error);
		return;
	}

	output->memory = vk_memory;
	output->offset = 0;
	output->size = req->size;
	output->property_flags = vk_allocator->memory_properties.memoryTypes[type_index].propertyFlags;
	output->source = vk_allocator;
	output->free = (toy_vulkan_free_fp)toy_free_vulkan_memory;
	output->padding = 0;

	toy_ok(error);
}


void toy_create_vulkan_memory_allocator (
	VkDevice dev,
	VkPhysicalDevice phy_dev,
	const VkAllocationCallbacks* vk_alc_cb,
	toy_memory_allocator_t* mem_alc,
	toy_vulkan_memory_allocator_t* output,
	toy_error_t* error)
{
	TOY_ASSERT(VK_NULL_HANDLE != dev && VK_NULL_HANDLE != phy_dev && NULL != output);

	toy_create_memory_pools(
		sizeof(toy_vulkan_memory_list_chunk_t) * 2048,
		sizeof(toy_vulkan_memory_list_chunk_t),
		mem_alc,
		&output->chunk_pools,
		&output->chunk_alc);
	if (toy_unlikely(NULL == output->chunk_pools)) {
		toy_err(TOY_ERROR_MEMORY_HOST_ALLOCATION_FAILED, "Malloc vulkan allocator chunk pool failed", error);
		return;
	}

	output->device = dev;
	vkGetPhysicalDeviceMemoryProperties(phy_dev, &output->memory_properties);
	output->mem_alc = mem_alc;

	if (NULL != vk_alc_cb) {
		output->vk_alc_cb = *vk_alc_cb;
		output->vk_alc_cb_p = &output->vk_alc_cb;
	}

	output->vk_list_alc.ctx = output;
	output->vk_list_alc.alloc = toy_alloc_vulkan_binding_memory_via_lists;
	output->vk_std_alc.ctx = output;
	output->vk_std_alc.alloc = toy_alloc_vulkan_memory;
}


void toy_destroy_vulkan_memory_allocator (
	toy_vulkan_memory_allocator_t* alc)
{
	toy_allocator_t* mem_alc = &alc->mem_alc->buddy_alc;

	for (int i = 0; i < VK_MAX_MEMORY_TYPES; ++i) {
		toy_vulkan_memory_list_p list = alc->vk_mem_list[i];
		while (NULL != list) {
			vkFreeMemory(alc->device, list->memory, alc->vk_alc_cb_p);
			toy_vulkan_memory_list_p next = list->next;
			toy_free_aligned(mem_alc, list);
			list = next;
		}
	}

	toy_destroy_memory_pools(alc->chunk_pools, alc->mem_alc);
}


void* toy_map_vulkan_memory (
	VkDevice dev,
	VkDeviceMemory memory,
	VkDeviceSize offset,
	VkDeviceSize size,
	VkMemoryPropertyFlags property_flags,
	toy_error_t* error)
{
	void* map_data = NULL;
	VkResult vk_err = vkMapMemory(dev, memory, offset, size, 0, &map_data);
	if (toy_unlikely(VK_SUCCESS != vk_err)) {
		toy_err_vkerr(TOY_ERROR_MEMORY_MAPPING_FAILED, vk_err, "Failed to map memory", error);
		return NULL;
	}
	if (0 == (property_flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
		VkMappedMemoryRange range;
		range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		range.pNext = NULL;
		range.memory = memory;
		range.offset = offset; // VkPhysicalDeviceLimits::nonCoherentAtomSize
		range.size = size;
		vk_err = vkInvalidateMappedMemoryRanges(dev, 1, &range);
		if (VK_SUCCESS != vk_err) {
			vkUnmapMemory(dev, memory);
			toy_err_vkerr(TOY_ERROR_MEMORY_FLUSH_FAILED, vk_err, "vkInvalidateMappedMemoryRanges failed", error);
			return NULL;
		}
	}

	toy_ok(error);
	return map_data;
}


void toy_unmap_vulkan_memory (
	VkDevice dev,
	VkDeviceMemory memory,
	VkDeviceSize offset,
	VkDeviceSize size,
	VkMemoryPropertyFlags property_flags,
	toy_error_t* error)
{
	VkResult vk_err = VK_SUCCESS;
	if (0 == (property_flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
		VkMappedMemoryRange range;
		range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		range.pNext = NULL;
		range.memory = memory;
		range.offset = offset; // VkPhysicalDeviceLimits::nonCoherentAtomSize
		range.size = size;
		vk_err = vkFlushMappedMemoryRanges(dev, 1, &range);
	}

	vkUnmapMemory(dev, memory);
	if (toy_likely(VK_SUCCESS == vk_err)) {
		toy_ok(error);
	}
	else {
		toy_err_vkerr(TOY_ERROR_MEMORY_FLUSH_FAILED, vk_err, "vkFlushMappedMemoryRanges failed", error);
	}
}
