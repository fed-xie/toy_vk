#include "../../include/platform/vulkan/toy_vulkan_image.h"

#include "../../toy_assert.h"

static VkSamplerAddressMode toy_image_sampler_wrap_to_vksampler_address_mode (enum toy_image_sampler_wrap_t wrap)
{
	switch (wrap)
	{
	case TOY_IMAGE_SAMPLER_WRAP_REPEAT:
		return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	case TOY_IMAGE_SAMPLER_WRAP_MIRRORED_REPEAT:
		return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	case TOY_IMAGE_SAMPLER_WRAP_CLAMP_TO_EDGE:
		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	case TOY_IMAGE_SAMPLER_WRAP_CLAMP_TO_BORDER:
		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	case TOY_IMAGE_SAMPLER_WRAP_MIRROR_CLAMP_TO_EDGE:
		return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
	default:
		TOY_ASSERT(0);
		return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	}
}

VkResult toy_create_vulkan_image_sampler (
	VkDevice dev,
	const toy_image_sampler_t* toy_sampler,
	const VkAllocationCallbacks* vk_alc_cb,
	VkSampler* output)
{
	TOY_ASSERT(
		TOY_IMAGE_SAMPLER_FILTER_NEAREST == toy_sampler->mag_filter ||
		TOY_IMAGE_SAMPLER_FILTER_LINEAR == toy_sampler->mag_filter);

	VkSamplerCreateInfo sampler_ci;
	sampler_ci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler_ci.pNext = NULL;
	sampler_ci.flags = 0;
	sampler_ci.magFilter = toy_sampler->mag_filter == TOY_IMAGE_SAMPLER_FILTER_NEAREST ?
		VK_FILTER_NEAREST : VK_FILTER_LINEAR;
	sampler_ci.minFilter = (toy_sampler->min_filter & 0xf) == TOY_IMAGE_SAMPLER_FILTER_NEAREST ?
		VK_FILTER_NEAREST : VK_FILTER_LINEAR;
	sampler_ci.mipmapMode = (toy_sampler->min_filter & (0xf << 4)) == TOY_IMAGE_SAMPLER_FILTER_LINEAR ?
		VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
	sampler_ci.addressModeU = toy_image_sampler_wrap_to_vksampler_address_mode(toy_sampler->wrap_u);
	sampler_ci.addressModeV = toy_image_sampler_wrap_to_vksampler_address_mode(toy_sampler->wrap_v);
	sampler_ci.addressModeW = toy_image_sampler_wrap_to_vksampler_address_mode(toy_sampler->wrap_w);
	sampler_ci.mipLodBias = 0.0f;
	sampler_ci.anisotropyEnable = VK_FALSE;
	sampler_ci.maxAnisotropy = 1.0f;
	sampler_ci.compareEnable = VK_FALSE;
	sampler_ci.compareOp = VK_COMPARE_OP_ALWAYS;
	sampler_ci.minLod = 0.0f;
	sampler_ci.maxLod = (float)TOY_MAX_VULKAN_MIPMAP_LAVEL;
	sampler_ci.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
	sampler_ci.unnormalizedCoordinates = VK_FALSE;
	return vkCreateSampler(dev, &sampler_ci, vk_alc_cb, output);
}


void toy_create_vulkan_image_depth (
	toy_vulkan_memory_allocator_p vk_allocator,
	VkFormat format,
	VkSampleCountFlagBits msaa_count,
	uint32_t width,
	uint32_t height,
	toy_vulkan_image_t* output,
	toy_error_t* error)
{
	VkDevice dev = vk_allocator->device;
	const VkAllocationCallbacks* vk_alc_cb = vk_allocator->vk_alc_cb_p;

	VkImageCreateInfo img_ci;
	img_ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	img_ci.pNext = NULL;
	img_ci.flags = 0;
	img_ci.imageType = VK_IMAGE_TYPE_2D;
	img_ci.format = format;
	img_ci.extent.width = width;
	img_ci.extent.height = height;
	img_ci.extent.depth = 1;
	img_ci.mipLevels = 1;
	img_ci.arrayLayers = 1;
	img_ci.samples = msaa_count;
	img_ci.tiling = VK_IMAGE_TILING_OPTIMAL;
	img_ci.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	img_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	img_ci.queueFamilyIndexCount = 0;
	img_ci.pQueueFamilyIndices = NULL;
	img_ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VkResult vk_err = vkCreateImage(dev, &img_ci, vk_alc_cb, &output->handle);
	if (VK_SUCCESS != vk_err) {
		toy_err_vkerr(TOY_ERROR_CREATE_OBJECT_FAILED, vk_err, "vkCreateImage for depth image failed", error);
		return;
	}

	const VkMemoryPropertyFlags property_flags[] = { VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0 };
	const uint32_t flag_count = sizeof(property_flags) / sizeof(*property_flags);

	toy_bind_vulkan_image_memory(
		dev, output->handle,
		property_flags, flag_count,
		&vk_allocator->memory_properties, &vk_allocator->vk_list_alc,
		&output->binding,
		error);
	if (toy_is_failed(*error)) {
		vkDestroyImage(dev, output->handle, vk_alc_cb);
		return;
	}

	VkImageViewCreateInfo view_ci;
	view_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_ci.pNext = NULL;
	view_ci.flags = 0;
	view_ci.image = output->handle;
	view_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
	view_ci.format = format;
	view_ci.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_ci.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_ci.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_ci.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	view_ci.subresourceRange.baseMipLevel = 0;
	view_ci.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
	view_ci.subresourceRange.baseArrayLayer = 0;
	view_ci.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
	vk_err = vkCreateImageView(dev, &view_ci, vk_alc_cb, &output->view);
	if (toy_unlikely(VK_SUCCESS != vk_err)) {
		toy_free_vulkan_memory_binding(&output->binding);
		vkDestroyImage(dev, output->handle, vk_alc_cb);
		toy_err_vkerr(TOY_ERROR_UNKNOWN_ERROR, vk_err, "vkCreateImageView for depth image failed", error);
		return;
	}

	toy_ok(error);
}


void toy_create_vulkan_image_texture2d (
	toy_vulkan_memory_allocator_p vk_allocator,
	VkFormat format,
	uint32_t width,
	uint32_t height,
	uint32_t mipmap_level,
	toy_vulkan_image_t* output,
	toy_error_t* error)
{
	VkDevice dev = vk_allocator->device;
	const VkAllocationCallbacks* vk_alc_cb = vk_allocator->vk_alc_cb_p;

	VkImageCreateInfo img_ci;
	img_ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	img_ci.pNext = NULL;
	img_ci.flags = 0;
	img_ci.imageType = VK_IMAGE_TYPE_2D;
	img_ci.format = format;
	img_ci.extent.width = width;
	img_ci.extent.height = height;
	img_ci.extent.depth = 1;
	img_ci.mipLevels = mipmap_level;
	img_ci.arrayLayers = 1;
	img_ci.samples = VK_SAMPLE_COUNT_1_BIT;
	img_ci.tiling = VK_IMAGE_TILING_OPTIMAL;
	img_ci.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	img_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	img_ci.queueFamilyIndexCount = 0;
	img_ci.pQueueFamilyIndices = NULL;
	img_ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VkResult vk_err = vkCreateImage(dev, &img_ci, vk_alc_cb, &output->handle);
	if (VK_SUCCESS != vk_err) {
		toy_err_vkerr(TOY_ERROR_CREATE_OBJECT_FAILED, vk_err, "vkCreateImage for depth image failed", error);
		return;
	}

	const VkMemoryPropertyFlags property_flags[] = { VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0 };
	const uint32_t flag_count = sizeof(property_flags) / sizeof(*property_flags);

	toy_bind_vulkan_image_memory(
		dev, output->handle,
		property_flags, flag_count,
		&vk_allocator->memory_properties, &vk_allocator->vk_list_alc,
		&output->binding,
		error);
	if (toy_is_failed(*error)) {
		vkDestroyImage(dev, output->handle, vk_alc_cb);
		return;
	}

	VkImageViewCreateInfo view_ci;
	view_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_ci.pNext = NULL;
	view_ci.flags = 0;
	view_ci.image = output->handle;
	view_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
	view_ci.format = format;
	view_ci.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_ci.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_ci.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_ci.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	view_ci.subresourceRange.baseMipLevel = 0;
	view_ci.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
	view_ci.subresourceRange.baseArrayLayer = 0;
	view_ci.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
	vk_err = vkCreateImageView(dev, &view_ci, vk_alc_cb, &output->view);
	if (toy_unlikely(VK_SUCCESS != vk_err)) {
		toy_free_vulkan_memory_binding(&output->binding);
		vkDestroyImage(dev, output->handle, vk_alc_cb);
		toy_err_vkerr(TOY_ERROR_UNKNOWN_ERROR, vk_err, "vkCreateImageView for texture2d failed", error);
		return;
	}

	toy_ok(error);
}


void toy_destroy_vulkan_image (
	VkDevice dev,
	toy_vulkan_image_t* image,
	const VkAllocationCallbacks* vk_alc_cb)
{
	vkDestroyImageView(dev, image->view, vk_alc_cb);
	toy_free_vulkan_memory_binding(&image->binding);
	vkDestroyImage(dev, image->handle, vk_alc_cb);
}
