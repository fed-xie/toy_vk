#include "../../include/auxiliary/render_pass/pass_base.h"

#include "../../toy_assert.h"

static VkResult toy_create_vulkan_descriptor_pool (
	VkDevice dev,
	const VkAllocationCallbacks* vk_alc_cb,
	VkDescriptorPool* output)
{
	// Todo: make desc_pool_size big enough
	VkDescriptorPoolSize desc_pool_sizes[3];
	desc_pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	desc_pool_sizes[0].descriptorCount = 16;
	desc_pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	desc_pool_sizes[1].descriptorCount = 16;
	desc_pool_sizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	desc_pool_sizes[2].descriptorCount = 16;
	const uint32_t pool_size_count = sizeof(desc_pool_sizes) / sizeof(desc_pool_sizes[0]);
	uint32_t max_sets = 0;
	for (uint32_t i = 0; i < pool_size_count; ++i)
		max_sets += desc_pool_sizes[i].descriptorCount;

	VkDescriptorPoolCreateInfo desc_pool_ci;
	desc_pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	desc_pool_ci.pNext = NULL;
	desc_pool_ci.flags = 0;
	desc_pool_ci.maxSets = max_sets;
	desc_pool_ci.poolSizeCount = pool_size_count;
	desc_pool_ci.pPoolSizes = desc_pool_sizes;
	return vkCreateDescriptorPool(dev, &desc_pool_ci, vk_alc_cb, output);
}


void toy_create_built_in_vulkan_frame_resource (
	toy_vulkan_device_t* vk_device,
	VkDeviceSize uniform_buffer_size,
	toy_vulkan_memory_allocator_p vk_allocator,
	const VkAllocationCallbacks* vk_alc_cb,
	toy_built_in_vulkan_frame_resource_t* output,
	toy_error_t* error)
{
	VkDevice dev = vk_device->handle;
	VkResult vk_err;
	vk_err = toy_create_vulkan_descriptor_pool(dev, vk_alc_cb, &output->descriptor_pool);
	if (toy_unlikely(VK_SUCCESS != vk_err)) {
		toy_err_vkerr(TOY_ERROR_CREATE_OBJECT_FAILED, vk_err, "toy_create_vulkan_descriptor_pool for frame resource failed", error);
		goto FAIL_DESC_POOL;
	}

	toy_create_vulkan_buffer(
		dev, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		uniform_buffer_size, &vk_device->physical_device.memory_properties,
		&vk_allocator->vk_list_alc, vk_allocator->vk_alc_cb_p,
		&output->uniform_stack.buffer,
		error);
	if (toy_is_failed(*error))
		goto FAIL_UNIFORM_BUFFER_STACK;
	toy_init_vulkan_buffer_stack(&output->uniform_stack.buffer, &output->uniform_stack);
	output->mapping_memory = NULL;

	VkCommandPoolCreateInfo cmd_pool_ci;
	cmd_pool_ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	cmd_pool_ci.pNext = NULL;
	cmd_pool_ci.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	cmd_pool_ci.queueFamilyIndex = vk_device->device_queue_families.graphic.family;
	TOY_ASSERT(VK_QUEUE_FAMILY_IGNORED != cmd_pool_ci.queueFamilyIndex);
	vk_err = vkCreateCommandPool(dev, &cmd_pool_ci, vk_alc_cb, &output->graphic_cmd_pool);
	if (toy_unlikely(VK_SUCCESS != vk_err)) {
		toy_err_vkerr(TOY_ERROR_CREATE_OBJECT_FAILED, vk_err, "vkCreateCommandPool for frame resource graphic_cmd_pool failed", error);
		goto FAIL_GRAPHIC_CMD_POOL;
	}

	TOY_ASSERT(VK_QUEUE_FAMILY_IGNORED != cmd_pool_ci.queueFamilyIndex);
	cmd_pool_ci.queueFamilyIndex = vk_device->device_queue_families.compute.family;
	vk_err = vkCreateCommandPool(dev, &cmd_pool_ci, vk_alc_cb, &output->compute_cmd_pool);
	if (toy_unlikely(VK_SUCCESS != vk_err)) {
		toy_err_vkerr(TOY_ERROR_CREATE_OBJECT_FAILED, vk_err, "vkCreateCommandPool for frame resource compute_cmd_pool failed", error);
		goto FAIL_COMPUTE_CMD_POOL;
	}

	toy_ok(error);
	return;

FAIL_COMPUTE_CMD_POOL:
	vkDestroyCommandPool(dev, output->graphic_cmd_pool, vk_alc_cb);
FAIL_GRAPHIC_CMD_POOL:
	vkDestroyBuffer(dev, output->uniform_stack.buffer.handle, vk_alc_cb);
	toy_free_vulkan_memory_binding(&output->uniform_stack.buffer.binding);
FAIL_UNIFORM_BUFFER_STACK:
	vkDestroyDescriptorPool(dev, output->descriptor_pool, vk_alc_cb);
FAIL_DESC_POOL:
	return;
}


void toy_destroy_built_in_vulkan_frame_resource (
	toy_vulkan_device_t* vk_device,
	toy_vulkan_memory_allocator_p vk_allocator,
	const VkAllocationCallbacks* vk_alc_cb,
	toy_built_in_vulkan_frame_resource_t* frame_res)
{
	VkDevice dev = vk_device->handle;
	toy_error_t err;

	vkDestroyCommandPool(dev, frame_res->compute_cmd_pool, vk_alc_cb);
	vkDestroyCommandPool(dev, frame_res->graphic_cmd_pool, vk_alc_cb);

	if (NULL != frame_res->mapping_memory) {
		toy_unmap_vulkan_buffer_memory(dev, &frame_res->uniform_stack.buffer, &err);
		frame_res->mapping_memory = NULL;
	}
	vkDestroyBuffer(dev, frame_res->uniform_stack.buffer.handle, vk_alc_cb);
	toy_free_vulkan_memory_binding(&frame_res->uniform_stack.buffer.binding);

	vkDestroyDescriptorPool(dev, frame_res->descriptor_pool, vk_alc_cb);
}
