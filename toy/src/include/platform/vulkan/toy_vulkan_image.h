#pragma once

#include "../../toy_platform.h"
#include "toy_vulkan_memory.h"
#include "../../toy_asset.h"


#define TOY_MAX_VULKAN_MIPMAP_LAVEL 8

typedef struct toy_vulkan_image_t {
	VkImage handle;
	VkImageView view;
	toy_vulkan_memory_binding_t binding;
}toy_vulkan_image_t, *toy_vulkan_image_p;


typedef struct toy_vulkan_sampler_t {
	toy_image_sampler_t params;
	VkSampler handle;
}toy_vulkan_sampler_t;


TOY_EXTERN_C_START

VkResult toy_create_vulkan_image_sampler (
	VkDevice dev,
	const toy_image_sampler_t* toy_sampler,
	const VkAllocationCallbacks* vk_alc_cb,
	VkSampler* output
);

void toy_create_vulkan_image_depth (
	toy_vulkan_memory_allocator_p vk_allocator,
	VkFormat format,
	VkSampleCountFlagBits msaa_count,
	uint32_t width,
	uint32_t height,
	toy_vulkan_image_t* output,
	toy_error_t* error
);

void toy_create_vulkan_image_texture2d (
	toy_vulkan_memory_allocator_p vk_allocator,
	VkFormat format,
	uint32_t width,
	uint32_t height,
	uint32_t mipmap_level,
	toy_vulkan_image_t* output,
	toy_error_t* error
);

void toy_destroy_vulkan_image (
	VkDevice dev,
	toy_vulkan_image_t* image,
	const VkAllocationCallbacks* vk_alc_cb
);

TOY_EXTERN_C_END
