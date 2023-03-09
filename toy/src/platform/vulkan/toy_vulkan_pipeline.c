#include "../../include/platform/vulkan/toy_vulkan_pipeline.h"

#include "../../include/toy_asset.h"
#include "../../toy_assert.h"


VkResult toy_create_vulkan_descriptor_set_layout (
	VkDevice dev,
	const VkDescriptorSetLayoutBinding* layout_bindings,
	uint32_t binding_count,
	const VkAllocationCallbacks* vk_alc_cb,
	toy_vulkan_descriptor_set_layout_t* output)
{
	if (VK_NULL_HANDLE != output->handle) {
		if ((output->binding_count == binding_count) && (0 == memcmp(output->layout_bindings, layout_bindings, sizeof(layout_bindings))))
			return VK_SUCCESS;
		return VK_ERROR_FORMAT_NOT_SUPPORTED;
	}

	VkDescriptorSetLayoutCreateInfo ci;
	ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	ci.pNext = NULL;
	// PUSH_DESCRIPTOR need extension VK_KHR_push_descriptor
	ci.flags = 0; // VkDescriptorSetLayoutCreateFlags: 0 | UPDATE_AFTER_BIND_POOL | PUSH_DESCRIPTOR
	ci.bindingCount = binding_count;
	ci.pBindings = layout_bindings;
	VkResult vk_err = vkCreateDescriptorSetLayout(dev, &ci, vk_alc_cb, &output->handle);
	if (VK_SUCCESS != vk_err) {
		output->handle = VK_NULL_HANDLE;
		return vk_err;
	}
	output->layout_bindings = layout_bindings;
	output->binding_count = ci.bindingCount;
	return VK_SUCCESS;
}


#define TOY_MAX_DESCRIPTOR_SET_BINDING 16 // 128 in specification but I don't need that much
void toy_update_vulkan_descriptor_set (
	VkDevice dev,
	VkDescriptorSet desc_set,
	toy_vulkan_descriptor_set_data_header_t* desc_set_data)
{
	uint32_t binding_count = desc_set_data->desc_set_layout->binding_count;
	const VkDescriptorSetLayoutBinding* layout_bindings = desc_set_data->desc_set_layout->layout_bindings;
	const toy_vulkan_descriptor_set_binding_t* bindings = desc_set_data->bindings;

	union {
		VkDescriptorImageInfo img_info;
		VkDescriptorBufferInfo buffer_info;
		VkBufferView buffer_view;
	} info[TOY_MAX_DESCRIPTOR_SET_BINDING];
	TOY_ASSERT(binding_count <= TOY_MAX_DESCRIPTOR_SET_BINDING);

	VkWriteDescriptorSet desc_set_writes[TOY_MAX_DESCRIPTOR_SET_BINDING];
	for (uint32_t i = 0; i < binding_count; ++i) {
		desc_set_writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		desc_set_writes[i].pNext = NULL;
		desc_set_writes[i].dstSet = desc_set;
		desc_set_writes[i].dstBinding = layout_bindings[i].binding;
		desc_set_writes[i].dstArrayElement = 0;
		desc_set_writes[i].descriptorCount = layout_bindings[i].descriptorCount;
		desc_set_writes[i].descriptorType = layout_bindings[i].descriptorType;
		switch (layout_bindings[i].descriptorType) {
		case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
		{
			toy_asset_pool_item_ref_t* image_ref = (toy_asset_pool_item_ref_t*)((uintptr_t)desc_set_data + bindings[i].offset);
			toy_asset_pool_item_ref_t* sampler_ref = image_ref + 1;
			toy_vulkan_image_p vk_image = toy_get_asset_item2(image_ref);
			toy_vulkan_sampler_t* vk_sampler = toy_get_asset_item2(sampler_ref);
			info[i].img_info.sampler = vk_sampler->handle;
			info[i].img_info.imageView = vk_image->view;
			info[i].img_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			desc_set_writes[i].pImageInfo = &info[i].img_info;
			desc_set_writes[i].pBufferInfo = NULL; // ignored with image sampler
			desc_set_writes[i].pTexelBufferView = NULL; // ignored with image sampler
			break;
		}
		case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
		case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
		case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
		case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
		{
			toy_vulkan_sub_buffer_p sub_buffer = (toy_vulkan_sub_buffer_p)((uintptr_t)desc_set_data + bindings[i].offset);
			info[i].buffer_info.buffer = sub_buffer->handle;
			info[i].buffer_info.offset = sub_buffer->offset;
			info[i].buffer_info.range = sub_buffer->size;
			desc_set_writes[i].pImageInfo = NULL; // ignored with uniform buffer
			desc_set_writes[i].pBufferInfo = &info[i].buffer_info;
			desc_set_writes[i].pTexelBufferView = NULL; // ignored with uniform buffer
			break;
		}
		default:
			TOY_ASSERT(0);
		}
	}

	vkUpdateDescriptorSets(dev, binding_count, desc_set_writes, 0, NULL);
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
