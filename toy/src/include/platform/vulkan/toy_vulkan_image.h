#pragma once

#include "../../toy_platform.h"
#include "toy_vulkan_memory.h"


typedef struct toy_vulkan_image_t {
	VkImage handle;
	VkImageView view;
	toy_vulkan_memory_binding_t binding;
}toy_vulkan_image_t, *toy_vulkan_image_p;


TOY_EXTERN_C_START

void toy_destroy_vulkan_image (
	VkDevice dev,
	toy_vulkan_image_t* image,
	const VkAllocationCallbacks* vk_alc_cb
);

TOY_EXTERN_C_END
