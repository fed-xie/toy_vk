#include "../../include/platform/vulkan/toy_vulkan_pipeline.h"

#include "../../include/toy_asset.h"
#include "../../toy_assert.h"


VkResult toy_create_vulkan_descriptor_set_layout (
	VkDevice dev,
	const VkDescriptorSetLayoutBinding* bindings,
	uint32_t binding_count,
	const VkAllocationCallbacks* vk_alc_cb,
	toy_vulkan_descriptor_set_layout_t* output)
{
	if (VK_NULL_HANDLE != output->handle) {
		if ((output->binding_count == binding_count) && (0 == memcmp(output->bindings, bindings, sizeof(bindings))))
			return VK_SUCCESS;
		return VK_ERROR_FORMAT_NOT_SUPPORTED;
	}

	VkDescriptorSetLayoutCreateInfo ci;
	ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	ci.pNext = NULL;
	// PUSH_DESCRIPTOR need extension VK_KHR_push_descriptor
	ci.flags = 0; // VkDescriptorSetLayoutCreateFlags: 0 | UPDATE_AFTER_BIND_POOL | PUSH_DESCRIPTOR
	ci.bindingCount = binding_count;
	ci.pBindings = bindings;
	VkResult vk_err = vkCreateDescriptorSetLayout(dev, &ci, vk_alc_cb, &output->handle);
	if (VK_SUCCESS != vk_err) {
		output->handle = VK_NULL_HANDLE;
		return vk_err;
	}
	output->bindings = bindings;
	output->binding_count = ci.bindingCount;
	return VK_SUCCESS;
}


VkShaderModule toy_create_vulkan_shader_module (
	const char* utf8_path,
	VkDevice dev,
	toy_vulkan_shader_loader_t* loader,
	const VkAllocationCallbacks* vk_alc_cb,
	toy_error_t* error)
{
	VkShaderModule handle;
	size_t code_size;

	toy_aligned_p file_content = toy_load_whole_file(
		utf8_path, loader->file_api, loader->alc, loader->tmp_alc, &code_size, error);
	if (toy_is_failed(*error))
		return VK_NULL_HANDLE;

	VkShaderModuleCreateInfo ci;
	ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	ci.pNext = NULL;
	ci.flags = 0;
	ci.codeSize = code_size;
	ci.pCode = (uint32_t*)file_content;
	VkResult vk_err = vkCreateShaderModule(dev, &ci, vk_alc_cb, &handle);
	toy_free_aligned(loader->alc, file_content);
	if (VK_SUCCESS != vk_err) {
		toy_err_vkerr(TOY_ERROR_CREATE_OBJECT_FAILED, vk_err, "vkCreateShaderModule failed", error);
		return VK_NULL_HANDLE;
	}

	toy_ok(error);
	return handle;
}
